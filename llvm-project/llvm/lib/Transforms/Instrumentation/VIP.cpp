#include "llvm/Transforms/Instrumentation/VIP.h"

#include "llvm/InitializePasses.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
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
  FunctionCallee vipWrite64Callee;
  FunctionCallee vipAssertCallee;
};

class VIPLegacyPass : public ModulePass {
 public:
  static char ID;
  VIPLegacyPass(): ModulePass(ID) {}
  bool runOnModule(Module &M) override;
};  

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
  vipWrite64Callee = M.getOrInsertFunction("vip_write64", FuncTy);
  vipAssertCallee = M.getOrInsertFunction("vip_assert", FuncTy);
}


bool VIPGuard::sanitizeFunction(Function &F) {
  bool Res = false;
  for (Instruction &I: instructions(F)) {
    if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
      Res |= instrumentFunctionPtrStore(SI);
    } else if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      Res |= instrumentIndirectCallSite(CI);
    }
  }
  return Res;
}

bool VIPGuard::instrumentFunctionPtrStore(StoreInst* SI) {
  Type* ValTy = SI->getValueOperand()->getType();
  PointerType* ValPtrTy = dyn_cast<PointerType>(ValTy);
  if (!ValPtrTy || !ValPtrTy->getElementType()->isFunctionTy()) {
    return false;
  }
  errs() << *SI << " will be instrumented\n"; 
  IRBuilder<> IRB(SI->getNextNode());
  IRB.CreateCall(vipWrite64Callee, {SI->getPointerOperand()});
  return true;
}

bool VIPGuard::instrumentIndirectCallSite(CallInst* CI) {
  if (!CI->isIndirectCall()) {
    return false;
  }
  return false;
}
