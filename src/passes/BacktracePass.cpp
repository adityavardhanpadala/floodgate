#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DataLayout.h"

using namespace llvm;

namespace {
  // Get size of a type in bytes
  unsigned getTypeSizeInBytes(Type *Ty, const DataLayout &DL) {
    if (Ty->isVoidTy()) return 0;
    return DL.getTypeAllocSize(Ty);
  }
  
  // Get a constant value for the default return value based on type
  Constant* getDefaultReturnValue(Type *Ty, Module *M) {
    if (Ty->isVoidTy()) return nullptr;
    
    if (Ty->isIntegerTy()) {
      return ConstantInt::get(Ty, 0);
    } else if (Ty->isFloatingPointTy()) {
      return ConstantFP::get(Ty, 0.0);
    } else if (Ty->isPointerTy()) {
      return ConstantPointerNull::get(cast<PointerType>(Ty));
    } else if (Ty->isStructTy()) {
      // For structs, create a struct with all fields zeroed
      StructType *ST = cast<StructType>(Ty);
      std::vector<Constant*> Elements;
      for (unsigned i = 0; i < ST->getNumElements(); i++) {
        Elements.push_back(getDefaultReturnValue(ST->getElementType(i), M));
      }
      return ConstantStruct::get(ST, Elements);
    } else if (Ty->isArrayTy()) {
      // For arrays, create an array with all elements zeroed
      ArrayType *AT = cast<ArrayType>(Ty);
      std::vector<Constant*> Elements(AT->getNumElements(), 
                       getDefaultReturnValue(AT->getElementType(), M));
      return ConstantArray::get(AT, Elements);
    } else if (Ty->isVectorTy()) {
      // For vectors, create a vector with all elements zeroed
      VectorType *VT = cast<VectorType>(Ty);
      Type *ElementTy = VT->getElementType();
      unsigned NumElements = VT->getElementCount().getKnownMinValue();
      std::vector<Constant*> Elements(NumElements, getDefaultReturnValue(ElementTy, M));
      return ConstantVector::get(Elements);
    }
    
    // For any other type, try to create a null/zero value
    return Constant::getNullValue(Ty);
  }

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
      DataLayout DL(M);
      
      // Get types we'll need
      Type *VoidTy = Type::getVoidTy(Ctx);
      Type *Int8Ty = Type::getInt8Ty(Ctx);
      Type *Int8PtrTy = PointerType::get(Int8Ty, 0);
      Type *Int1Ty = Type::getInt1Ty(Ctx);
      Type *Int32Ty = Type::getInt32Ty(Ctx);
      Type *Int64Ty = Type::getInt64Ty(Ctx);
      Type *IntPtrTy = DL.getIntPtrType(Ctx);
      
      // Create necessary function declarations if they don't exist
      
      // register_patchable_function(uintptr_t func_addr, const char *func_name, uint8_t return_type_size, void *default_return_value)
      FunctionType *RegisterFnTy = FunctionType::get(VoidTy, 
                                   {IntPtrTy, Int8PtrTy, Int8Ty, Int8PtrTy}, false);
      FunctionCallee RegisterFn = M->getOrInsertFunction("register_patchable_function", RegisterFnTy);
      
      // int should_execute_function(uintptr_t func_addr)
      FunctionType *ShouldExecFnTy = FunctionType::get(Int32Ty, {IntPtrTy}, false);
      FunctionCallee ShouldExecFn = M->getOrInsertFunction("should_execute_function", ShouldExecFnTy);
      
      // void* get_default_return_value(uintptr_t func_addr)
      FunctionType *GetDefaultFnTy = FunctionType::get(Int8PtrTy, {IntPtrTy}, false);
      FunctionCallee GetDefaultFn = M->getOrInsertFunction("get_default_return_value", GetDefaultFnTy);
      
      // Initialize patching system if not already done (only once per module)
      static bool initialized = false;
      if (!initialized) {
        // Create global constructor to register all functions
        Function *Ctor = Function::Create(FunctionType::get(VoidTy, false),
                                         GlobalValue::InternalLinkage,
                                         "floodgate.module.init", M);
        Ctor->setSection(".text.startup");
        
        // Create init basic block
        BasicBlock *BB = BasicBlock::Create(Ctx, "entry", Ctor);
        IRBuilder<> Builder(BB);
        
        // Initialize the patch system
        FunctionType *InitFnTy = FunctionType::get(VoidTy, {Int8PtrTy}, false);
        FunctionCallee InitFn = M->getOrInsertFunction("initialize_patch_system", InitFnTy);
        
        // Create global string with patch file path
        Constant *PatchFilePath = Builder.CreateGlobalStringPtr("/tmp/floodgate.patch");
        
        // Call init function
        Builder.CreateCall(InitFn, {PatchFilePath});
        Builder.CreateRetVoid();
        
        // Create global variable to ensure constructor runs
        // Add to global ctors
        std::vector<Type*> formals;
        FunctionType* CtorFTy = FunctionType::get(Type::getVoidTy(Ctx), formals, false);
        
        // Create the ctor/dtor global array
        ArrayType* AT = ArrayType::get(
            StructType::get(
                Int32Ty,
                PointerType::get(CtorFTy, 0),
                Int8PtrTy
            ), 
            1
        );
        
        // Priority is 0, function is Ctor
        Constant* CTors = ConstantArray::get(AT, {
            ConstantStruct::get(
                cast<StructType>(AT->getElementType()),
                {
                    ConstantInt::get(Int32Ty, 0),
                    ConstantExpr::getBitCast(Ctor, PointerType::get(CtorFTy, 0)),
                    ConstantPointerNull::get(cast<PointerType>(Int8PtrTy))
                }
            )
        });
        
        // Create a global variable with the array
        auto *GV = new GlobalVariable(
            *M, 
            AT,
            false, 
            GlobalValue::AppendingLinkage,
            CTors,
            "llvm.global_ctors"
        );
        
        initialized = true;
      }
      
      // Register this function with the patching system
      // We'll add this to the module constructor
      Function *Ctor = M->getFunction("floodgate.module.init");
      if (Ctor) {
        // Get the entry block
        BasicBlock &EntryBB = Ctor->getEntryBlock();
        // Get the terminator instruction
        Instruction *Terminator = EntryBB.getTerminator();
        // Create builder before the terminator
        IRBuilder<> InitBuilder(&EntryBB, Terminator->getIterator());
        
        // Get function address
        Constant *FuncAddr = ConstantExpr::getPtrToInt(&F, IntPtrTy);
        
        // Get function name
        Constant *FuncNameStr = InitBuilder.CreateGlobalStringPtr(F.getName());
        
        // Get return type size
        Type *RetTy = F.getReturnType();
        uint8_t RetSize = getTypeSizeInBytes(RetTy, DL);
        Constant *RetSizeConst = ConstantInt::get(Int8Ty, RetSize);
        
        // Create default return value
        Value *DefaultRetPtr = nullptr;
        if (!RetTy->isVoidTy()) {
          Constant *DefaultRet = getDefaultReturnValue(RetTy, M);
          GlobalVariable *DefaultRetGlobal = new GlobalVariable(
              *M, RetTy, true, GlobalValue::PrivateLinkage, DefaultRet,
              "default_return." + F.getName());
          DefaultRetGlobal->setAlignment(MaybeAlign(DL.getTypeAllocSize(RetTy)));
          DefaultRetPtr = InitBuilder.CreateBitCast(DefaultRetGlobal, Int8PtrTy);
        } else {
          DefaultRetPtr = ConstantPointerNull::get(cast<PointerType>(Int8PtrTy));
        }
        
        // Call register function
        InitBuilder.CreateCall(RegisterFn, {FuncAddr, FuncNameStr, RetSizeConst, DefaultRetPtr});
      }
      
      // Now instrument the function entry
      BasicBlock &EntryBB = F.getEntryBlock();
      
      // Split the entry block to insert our check
      BasicBlock *CheckBB = BasicBlock::Create(Ctx, "patch.check", &F, &EntryBB);
      BasicBlock *OriginalEntryBB = &EntryBB;
      BasicBlock *SkipBB = nullptr;
      
      // If the function returns a value, create a skip block with the default return
      if (!F.getReturnType()->isVoidTy()) {
        SkipBB = BasicBlock::Create(Ctx, "patch.skip", &F);
        IRBuilder<> SkipBuilder(SkipBB);
        
        // Get function address for the check
        Constant *FuncAddr = ConstantExpr::getPtrToInt(&F, IntPtrTy);
        
        // Call get_default_return_value
        Value *DefaultRetPtr = SkipBuilder.CreateCall(GetDefaultFn, {FuncAddr});
        
        // Load and cast the default value to the right type
        Value *CastedPtr = SkipBuilder.CreateBitCast(DefaultRetPtr, 
                                       PointerType::get(F.getReturnType(), 0));
        Value *DefaultRet = SkipBuilder.CreateLoad(F.getReturnType(), CastedPtr);
        
        // Return the default value
        SkipBuilder.CreateRet(DefaultRet);
      } else {
        // For void functions, just create a simple early return
        SkipBB = BasicBlock::Create(Ctx, "patch.skip", &F);
        IRBuilder<> SkipBuilder(SkipBB);
        SkipBuilder.CreateRetVoid();
      }
      
      // Update branch instructions in predecessors
      for (auto PI = pred_begin(&EntryBB), E = pred_end(&EntryBB); PI != E;) {
        BasicBlock *Pred = *PI++;
        BranchInst *BI = dyn_cast<BranchInst>(Pred->getTerminator());
        if (BI && BI->isUnconditional() && BI->getSuccessor(0) == &EntryBB) {
          BI->setSuccessor(0, CheckBB);
        }
      }
      
      // Create the check in the new block
      IRBuilder<> CheckBuilder(CheckBB);
      
      // Get function address for the check
      Constant *FuncAddr = ConstantExpr::getPtrToInt(&F, IntPtrTy);
      
      // Call should_execute_function
      Value *ShouldExec = CheckBuilder.CreateCall(ShouldExecFn, {FuncAddr});
      
      // Convert to boolean
      Value *ShouldExecBool = CheckBuilder.CreateICmpNE(ShouldExec, 
                                           ConstantInt::get(Int32Ty, 0));
      
      // Branch based on result
      CheckBuilder.CreateCondBr(ShouldExecBool, OriginalEntryBB, SkipBB);
      
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

static RegisterPass<BacktracePass> X("backtrace", "Function Patching Instrumentation Pass");

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