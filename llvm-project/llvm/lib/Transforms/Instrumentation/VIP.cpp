#include "llvm/Transforms/Instrumentation/VIP.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class VIPGuard {
 public:
  bool sanitizeModule(Module&);
  bool sanitizeFunction(Function&);
 private:
  void prepareVIPLibraryCallee(Module&);
  bool instrumentFunctionPtrStore(StoreInst*);
  bool instrumentIndirectCallSite(CallInst*);
  bool instrumentIndirectCallSiteWithDebug(CallInst*);
  bool instrumentInitializedGlobals(Module&);
  

  // some helper function
  bool generateCtorsForInitializedGlobals(GlobalVariable*, Type*, unsigned=0);
  bool isFunctionPointerType(Type*);
  bool isVirtualFunctionCall(CallInst*);
  MDNode* vipSignature;
  FunctionCallee vipWrite64Callee;
  FunctionCallee vipPendingWrite64Callee;
  FunctionCallee vipAssertCallee;
  FunctionCallee vipAssertDebugCallee;
};

class VIPLegacyPass : public ModulePass {
 public:
  static char ID;
  VIPLegacyPass(): ModulePass(ID) {}
  bool runOnModule(Module &M) override;
};  

int linear_id(void) {
  static int i = 0;
  return i++;
}

} // anonymous namesapce


char VIPLegacyPass::ID = 0;
INITIALIZE_PASS(VIPLegacyPass, "vip",
                "Value Invariant Properties", false, false)

ModulePass* llvm::createVIPLegacyPassPass() {
  return new VIPLegacyPass();
}

bool VIPLegacyPass::runOnModule(Module &M) {
  VIPGuard vipGuard;
  return vipGuard.sanitizeModule(M);
}

PreservedAnalyses VIPPass::run(Module &M, ModuleAnalysisManager &AM) {
  VIPGuard vipGuard;
  if (vipGuard.sanitizeModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}

bool VIPGuard::sanitizeModule(Module &M) {
  bool Res = false;
  prepareVIPLibraryCallee(M);
  instrumentInitializedGlobals(M);
  for (Function &F: M) {
    Res |= sanitizeFunction(F);
  }
  return Res;
}

void VIPGuard::prepareVIPLibraryCallee(Module &M) {
  LLVMContext &Ctx = M.getContext();
  FunctionType* FuncTy = FunctionType::get(Type::getVoidTy(Ctx),
                                           {Type::getInt8PtrTy(Ctx)},
                                           false);
  FunctionType* DebugFuncTy = FunctionType::get(Type::getVoidTy(Ctx),
                                                {Type::getInt8PtrTy(Ctx), 
                                                 Type::getInt8PtrTy(Ctx)},
                                                false);
  vipWrite64Callee = M.getOrInsertFunction("vip_write64", FuncTy);
  vipPendingWrite64Callee = M.getOrInsertFunction("vip_pending_write64", FuncTy);
  vipAssertCallee = M.getOrInsertFunction("vip_assert", FuncTy);
  vipAssertDebugCallee = M.getOrInsertFunction("vip_assert_debug", DebugFuncTy);
  vipSignature = MDNode::get(Ctx, MDString::get(Ctx, "vip_signature"));
}


bool VIPGuard::sanitizeFunction(Function &F) {
  bool Res = false;
  for (Instruction &I: instructions(F)) {
    if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
      Res |= instrumentFunctionPtrStore(SI);
    } else if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      // Res |= instrumentIndirectCallSite(CI);
      Res |= instrumentIndirectCallSiteWithDebug(CI);
    }
  }
  return Res;
}

bool VIPGuard::instrumentFunctionPtrStore(StoreInst* SI) {
  Type* ValTy = SI->getValueOperand()->getType();
  if (!isFunctionPointerType(ValTy)) {
    return false;
  }
  // errs() << *SI << " will be instrumented\n"; 
  IRBuilder<> IRB(SI->getNextNode());
  // errs() << "write64 correct arg:" << *SI->getPointerOperand() << "\n";
  CallInst* CI = IRB.CreateCall(vipWrite64Callee, {SI->getPointerOperand()});
  CI->setMetadata("vip_signature", vipSignature);
  return true;
}

bool VIPGuard::instrumentIndirectCallSite(CallInst* CI) {
  if (!CI->isIndirectCall()) {
    return false;
  }
  if (isVirtualFunctionCall(CI)) {
    return false;
  }
  IRBuilder<> IRB(CI);
  Value* Callee = CI->getCalledValue();
  Value* PtrToCallee = NULL;
  if (LoadInst* LI = dyn_cast<LoadInst>(Callee)) {
    PtrToCallee = LI->getPointerOperand();
  } else {
    // unexpected type
    return false;
  }
  IRB.CreateCall(vipAssertCallee, {PtrToCallee});
  return true;
}

bool VIPGuard::instrumentIndirectCallSiteWithDebug(CallInst* CI) {
  Function* F = CI->getParent()->getParent();
  if (!CI->isIndirectCall()) {
    return false;
  }
  if (isVirtualFunctionCall(CI)) {
    return false;
  }
  IRBuilder<> IRB(CI);
  Value* Callee = CI->getCalledValue();
  Value* PtrToCallee = NULL;
  if (LoadInst* LI = dyn_cast<LoadInst>(Callee)) {
    PtrToCallee = LI->getPointerOperand();
  } else {
    return false;
  }
  // errs() << *CI << " is instrumented!\n";
  std::string SrcFile = CI->getModule()->getSourceFileName();
  std::string s = SrcFile + ":" + F->getName().str();
  StringRef CallerStr = StringRef(s);
  Constant* CallerStrGV = IRB.CreateGlobalStringPtr(CallerStr);
  IRB.CreateCall(vipAssertDebugCallee, {PtrToCallee, CallerStrGV});
  return true;
}

bool VIPGuard::instrumentInitializedGlobals(Module &M) {
  /*LLVMContext &Ctx = M.getContext();
  FunctionType* FTy = FunctionType::get(Type::getVoidTy(Ctx),
                                        {Type::getVoidTy(Ctx)}, false);
  Function* HelloFunc = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                         "vip_hello", M);
  IRBuilder<> IRB(BasicBlock::Create(Ctx, "", HelloFunc));
  FunctionType* PutsFTy = FunctionType::get(IRB.getInt32Ty(), 
                                            {IRB.getInt8PtrTy()}, false);
  FunctionCallee PutsFunc = M.getOrInsertFunction("puts", PutsFTy);
  Constant* HelloStrGV = IRB.CreateGlobalStringPtr("[VIP says HELLO]");
  IRB.CreateCall(PutsFunc, {HelloStrGV});
  appendToGlobalCtors(M, HelloFunc, 0);
  IRB.CreateRetVoid();*/
  


  // HelloFunc->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
  SmallVector<GlobalVariable*, 32> GlobalsToProtect;
  for (GlobalVariable &GV: M.globals()) {
    Type* GVTy = GV.getType();    
    PointerType *GVPtrTy = dyn_cast<PointerType>(GVTy);
    if (!GVPtrTy) {
      continue;
    }
    Type* GVElementTy = GVPtrTy->getElementType();
    if (isFunctionPointerType(GVElementTy)) {
      generateCtorsForInitializedGlobals(&GV, GVElementTy);
    } else if (GVElementTy->isStructTy()) {
      for (unsigned i = 0; i < GVElementTy->getStructNumElements(); ++i) {
        if (isFunctionPointerType(GVElementTy->getStructElementType(i))) {
          generateCtorsForInitializedGlobals(&GV, GVElementTy, i);
        }
      }
    } else {
      // others
    }
  }

  return true;
}

bool VIPGuard::generateCtorsForInitializedGlobals(GlobalVariable *GV,
                                                  Type* ElementTy,
                                                  unsigned index) {
  Module* M = GV->getParent();
  LLVMContext &Ctx = M->getContext();
  
  FunctionType* NewCtorFTy = FunctionType::get(Type::getVoidTy(Ctx),
                                              {Type::getVoidTy(Ctx)},
                                              false);
  std::string NewCtorName = "vip_init_gv_";
  if (GV->hasName()) {
    NewCtorName += M->getName().str() + GV->getName().str() + std::to_string(index);
  } else {
    NewCtorName += M->getName().str() + std::to_string(linear_id());
  }
  Function* NewCtorFunc = Function::Create(NewCtorFTy, 
                                           GlobalValue::ExternalLinkage,
                                            NewCtorName, *M);
  IRBuilder<> IRB(BasicBlock::Create(Ctx, "", NewCtorFunc));  
  // Value* GVElement = IRB.CreateGEP(GV->getType(), GV);
  Value* Ptr = nullptr;
    // errs() << *(GV->getType()) << "is GV type\n";
  if (isFunctionPointerType(ElementTy)) {
    Ptr = GV;
  } else if (ElementTy->isStructTy()) {
    Ptr = IRB.CreateStructGEP(ElementTy, GV, index);
    // Ptr = IRB.CreateInBoundsGEP(ElementTy, GV, IRB.getInt32(index));
  } else {
    errs() << *GV << " not instrumented\n";
    return false;
  }
  // errs() << "Ptr: " << *Ptr << "\n";
  IRB.CreateCall(vipPendingWrite64Callee, {Ptr});
  IRB.CreateRetVoid();
  appendToGlobalCtors(*M, NewCtorFunc, 0);
  errs() << NewCtorName << " has been created\n";
  return true;
}



bool VIPGuard::isFunctionPointerType(Type* Ty) {
  PointerType* PtrTy = dyn_cast<PointerType>(Ty);
  return PtrTy && PtrTy->getElementType()->isFunctionTy();
}


bool VIPGuard::isVirtualFunctionCall(CallInst* CI) {
  if (!CI->isIndirectCall()) {
    return false;
  }
  Value* callee = CI->getCalledOperand();
  if (LoadInst* LI = dyn_cast<LoadInst>(callee)) {
    return LI->hasMetadata("vfptr_flag");
  }
  return false;
}


