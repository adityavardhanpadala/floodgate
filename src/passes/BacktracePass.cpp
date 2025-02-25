#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
  // Legacy pass manager version
  struct BacktracePass : public FunctionPass {
    static char ID;
    BacktracePass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) override {
      return instrumentFunction(F);
    }
    
    bool instrumentFunction(Function &F) {
      // Skip declarations and intrinsics
      if (F.isDeclaration())
        return false;
      
      Module *M = F.getParent();
      
      LLVMContext &Ctx = M->getContext();
      Type *VoidTy = Type::getVoidTy(Ctx);
      Type *Int8Ty = Type::getInt8Ty(Ctx);
      Type *Int8PtrTy = PointerType::get(Int8Ty, 0);
      FunctionType *BacktraceFnTy = FunctionType::get(VoidTy, {Int8PtrTy}, false);
      FunctionCallee PrintBacktraceFn = M->getOrInsertFunction("print_backtrace_if_enabled", BacktraceFnTy);
      
      // Get the first basic block and its first instruction
      BasicBlock &EntryBB = F.getEntryBlock();
      Instruction &FirstInst = *EntryBB.getFirstInsertionPt();
      
      IRBuilder<> Builder(&FirstInst);
      
      Constant *FuncNameStr = Builder.CreateGlobalStringPtr(F.getName());
      
      Builder.CreateCall(PrintBacktraceFn, {FuncNameStr});
      
      return true;
    }
  };

  struct BacktraceFunctionPass : public PassInfoMixin<BacktraceFunctionPass> {
    static bool isRequired() { return true; }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
      BacktracePass LegacyPass;
      if (!LegacyPass.instrumentFunction(F))
        return PreservedAnalyses::all();
      return PreservedAnalyses::none();
    }
  };
}

char BacktracePass::ID = 0;

static RegisterPass<BacktracePass> X("backtrace", "Backtrace Function Instrumentation Pass");

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "BacktracePass", LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "backtrace") {
            FPM.addPass(BacktraceFunctionPass());
            return true;
          }
          return false;
        });
    }
  };
}
