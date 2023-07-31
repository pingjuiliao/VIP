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

  enum GEPType {
    geptype_error=0,
    geptype_struct,
    geptype_array,
    NUM_GEPTYPE
  };
  struct GEPEncode {
    GEPEncode(): type_enc(geptype_error), index0(-1), index1(-1) {}
    GEPType type_enc;
    int index0; 
    int index1;
  };
  // some helper function
  void traverseSensitiveGlobals(GlobalVariable*, 
                                SmallVector<GEPEncode*, 16>,
                                Type*);
  void generateCtorsForSensitiveGlobals(GlobalVariable*, 
                                        SmallVector<GEPEncode*, 16>);
  bool isFunctionPointerType(Type*);
  bool isVirtualFunctionCall(CallInst*);
  std::string createCtorName(Value*, Module*, SmallVector<GEPEncode*, 16>);
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
      Res |= instrumentIndirectCallSite(CI);
      // Res |= instrumentIndirectCallSiteWithDebug(CI);
    }
  }
  return Res;
}

bool VIPGuard::instrumentFunctionPtrStore(StoreInst* SI) {
  Type* ValTy = SI->getValueOperand()->getType();
  if (!isFunctionPointerType(ValTy)) {
    return false;
  }
  IRBuilder<> IRB(SI->getNextNode());
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
  std::string SrcFile = CI->getModule()->getSourceFileName();
  std::string s = SrcFile + ":" + F->getName().str();
  StringRef CallerStr = StringRef(s);
  Constant* CallerStrGV = IRB.CreateGlobalStringPtr(CallerStr);
  IRB.CreateCall(vipAssertDebugCallee, {PtrToCallee, CallerStrGV});
  return true;
}

bool VIPGuard::instrumentInitializedGlobals(Module &M) {
  SmallVector<GEPEncode*, 16> GEPEncStack;

  for (GlobalVariable &GV: M.globals()) {
    Type* GVTy = GV.getType();
    if (GV.getName().startswith("llvm")) {
      continue;
    }
    PointerType *GVPtrTy = dyn_cast<PointerType>(GVTy);
    if (!GVPtrTy) {
      continue;
    }
    Type* GVElementTy = GVPtrTy->getElementType();
    GEPEncStack.clear();
    errs() << "checking GV " << GV.getName() << "\n";
    traverseSensitiveGlobals(&GV, GEPEncStack, GVElementTy);
  }

  return true;
}

void VIPGuard::traverseSensitiveGlobals(GlobalVariable *GV,       
                                        SmallVector<GEPEncode*, 16> EncStack, 
                                        Type* CurrType) {
  if (isFunctionPointerType(CurrType)) {
    generateCtorsForSensitiveGlobals(GV, EncStack);
  } else if (CurrType->isStructTy()) {
    GEPEncode* encoded = new GEPEncode();
    encoded->type_enc = geptype_struct;
    // some helper function
    for (unsigned i = 0; i < CurrType->getStructNumElements(); ++i) {
      encoded->index0 = i;
      EncStack.push_back(encoded);
      traverseSensitiveGlobals(GV, 
                               EncStack, 
                               CurrType->getStructElementType(i));
      EncStack.pop_back();
    }
    delete encoded;
  } else if (CurrType->isArrayTy()) {
    GEPEncode* encoded = new GEPEncode();
    encoded->type_enc = geptype_array;
    for (uint64_t i = 0; i < CurrType->getArrayNumElements(); ++i) {
      encoded->index0 = i;
      EncStack.push_back(encoded);
      traverseSensitiveGlobals(GV, EncStack,
                               CurrType->getArrayElementType());  
      EncStack.pop_back();
    } 
    delete encoded; 
  }
}

void VIPGuard::generateCtorsForSensitiveGlobals(
    GlobalVariable* GV,
    SmallVector<GEPEncode*, 16> EncStack) {
  Module* M = GV->getParent();
  LLVMContext &Ctx = M->getContext();

  FunctionType* NewCtorFuncTy = FunctionType::get(Type::getVoidTy(Ctx),
                                                  {Type::getVoidTy(Ctx)},
                                                  false);
  std::string NewCtorName = createCtorName(GV, M, EncStack);
  Function* NewCtorFunc = Function::Create(NewCtorFuncTy, 
                                           GlobalValue::ExternalLinkage,
                                           NewCtorName, *M);

  // debug
  /*errs() << "[ ";
  for (auto gepEncoded: EncStack) {
    errs() << "(" << gepEncoded->type_enc << ", " << gepEncoded->index0 
           << "), ";
  }
  errs() << "]\n";*/

  // building constructor body
  IRBuilder<> IRB(BasicBlock::Create(Ctx, "", NewCtorFunc));
  PointerType* GVPtrTy = dyn_cast<PointerType>(GV->getType());
  Type* ElemTy = GVPtrTy->getElementType();
  Value* Ptr = GV;
  for (auto gepEncoded: EncStack) {
    switch (gepEncoded->type_enc) {
      case geptype_struct: {
        Ptr = IRB.CreateStructGEP(ElemTy, Ptr, gepEncoded->index0);
        ElemTy = ElemTy->getStructElementType(gepEncoded->index0); 
        break;
      }
      case geptype_array: {
        SmallVector<Value*, 4> Args;
        Args.push_back(IRB.getInt64(0));
        Args.push_back(IRB.getInt64(gepEncoded->index0));
        if (gepEncoded->index1 >= 0) {
          Args.push_back(IRB.getInt64(gepEncoded->index1));
        }
        Ptr = IRB.CreateInBoundsGEP(ElemTy, Ptr, Args);
        ElemTy = ElemTy->getArrayElementType();
        break;
      }
      default: {
        break;  
      }
    }
  }
  IRB.CreateCall(vipPendingWrite64Callee, {Ptr});
  IRB.CreateRetVoid();
  appendToGlobalCtors(*M, NewCtorFunc, 0);
  errs() << NewCtorName << " has been created\n";
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

std::string VIPGuard::createCtorName(Value* Val, Module* M,
                                     SmallVector<GEPEncode*, 16> EncStack) {
  std::string FullName = "vip_init_gv_";
  std::string ModuleName = M->getName().str();
  std::string ValName;
  if (Val->hasName()) {
    ValName = Val->getName().str();
  } else {
    ValName = "valueid" + std::to_string(linear_id());
  }
  FullName += ModuleName + "_" + ValName;
  // FullName.replace(FullName.begin(), FullName.end(), ".", "_");
  for (auto Encoded: EncStack) {
    FullName += "_" + std::to_string(Encoded->index0);
    if (Encoded->index1 >= 0) {
      FullName += "-" + std::to_string(Encoded->index1);  
    }
  }
  return FullName;
}
