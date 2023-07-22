#include "llvm/Transforms/Instrumentation/VIP.h"

#include "llvm/InitializePasses.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

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
  for (auto &F: M) {
    errs() << F.getName() << "\n";
  }
  return false;
}


PreservedAnalyses VIPPass::run(Module &M, ModuleAnalysisManager &AM) {
  LLVMContext& Ctx = M.getContext();
  FunctionCallee PutsFn = M.getOrInsertFunction("puts", 
                                                Type::getInt32Ty(Ctx), 
                                                Type::getInt8PtrTy(Ctx));
  for (Function &F: M) {
    // errs() << F.getName() << "\n";
    IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    Constant* HelloStr = IRB.CreateGlobalStringPtr("llvm pass say hello");
    LoadInst* LdHello = IRB.CreateLoad(IRB.getInt8PtrTy(), HelloStr);
    IRB.CreateCall(PutsFn, {LdHello});
  }
  return PreservedAnalyses::none();
}
