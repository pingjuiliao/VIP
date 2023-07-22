#ifndef LLVM_TRANSFOMRS_INSTRUMENTATION_H
#define LLVM_TRANSFOMRS_INSTRUMENTATION_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

ModulePass *createVIPLegacyPassPass();

class VIPPass : public PassInfoMixin<VIPPass> {
 public:
  PreservedAnalyses run(Module&, ModuleAnalysisManager &AM);
};

} // namespace llvm
#endif  // LLVM_TRANSFOMRS_INSTRUMENTATION_H
