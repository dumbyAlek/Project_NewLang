// main.cpp
#include <iostream>
#include <string>
#include "include/headers.h"
#include "include/KaleidoscopeJIT.h"
#include "include/llvm_all.h"
#include "include/codeGen.h"
#include "include/runtime.h"

using namespace std;
using namespace llvm;
using namespace llvm::orc;

std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;

static std::unique_ptr<KaleidoscopeJIT> TheJIT;
static ExitOnError ExitOnErr;
enum ParseState { OUTSIDE, INCLUDES, IN_BODY };
std::vector<std::string> Includes;

// llvm::ExitOnError<std::unique_ptr<llvm::orc::KaleidoscopeJIT>> ExitOnErr;

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndManagers() {
  // Create a shared context (used by ORC ThreadSafeModule)
  TheContext = std::make_unique<llvm::LLVMContext>();

  // Create a new module in that context
  TheModule = std::make_unique<llvm::Module>("msk2", *TheContext);

  // If TheJIT exists, use its data layout
  if (TheJIT)
    TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module
  Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();

  // StandardInstrumentations wants an LLVMContext& parameter
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                     /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes to FunctionPassManager (note: for new PassManager API
  // you will want to use PassBuilder to create FunctionPassManager with module)
  TheFPM->addPass(InstCombinePass());
  TheFPM->addPass(ReassociatePass());
  TheFPM->addPass(GVNPass());
  TheFPM->addPass(SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}


static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      // ExitOnErr(TheJIT->addModule(
      //     ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
      // InitializeModuleAndManagers();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void HandleTopLevelExpression() {
    if (auto FnAST = ParseTopLevelExpr()) {
        // Generate a unique name for this top-level expression
        static int anonCount = 0;
        std::string anonName = "__anon_expr" + std::to_string(anonCount++);
        FnAST->getProto()->setName(anonName);

        if (FnAST->codegen()) {
            auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
            auto RT = TheJIT->getMainJITDylib().createResourceTracker();
            ExitOnErr(TheJIT->addModule(std::move(TSM)));

            // Lookup by our unique name
            auto ExprSymbol = ExitOnErr(TheJIT->lookup(anonName));
            double (*FP)() = reinterpret_cast<double (*)()>(ExprSymbol.getAddress().getValue());

            fprintf(stderr, "Evaluated to %f\n", FP());

            ExitOnErr(RT->remove());

            // Recreate module/context for the next expression
            TheContext = std::make_unique<LLVMContext>();
            TheModule  = std::make_unique<Module>("msk2", *TheContext);
            Builder    = std::make_unique<IRBuilder<>>(*TheContext);
        }
    } else {
        getNextToken();
    }
}



/// top ::= definition | external | expression | ';'
static void MainLoop() {
    ParseState state = OUTSIDE;

    while (true) {
        switch(state) {
            case OUTSIDE:
                switch(CurrentToken) {
                    case tok_eof:
                        return;
                    case tok_beginInc:
                        state = INCLUDES;
                        getNextToken();
                        break;
                    case tok_beginBody:
                        state = IN_BODY;
                        getNextToken();
                        break;
                    default:
                        // skip anything else outside
                        getNextToken();
                        break;
                }
                break;

            case INCLUDES:
                switch(CurrentToken) {
                    case tok_endInc:
                        state = OUTSIDE;
                        getNextToken();
                        break;
                    case tok_identifier:
                        // Optional: store includes if needed
                        Includes.push_back(IdentifierStr);
                        getNextToken();
                        break;
                    default:
                        getNextToken(); // skip unknown tokens inside includes
                        break;
                }
                break;

            case IN_BODY:
                switch(CurrentToken) {
                    case tok_endBody:
                        state = OUTSIDE;
                        getNextToken();
                        break;
                    case tok_def:
                        HandleDefinition();
                        break;
                    case tok_extern:
                        HandleExtern();
                        break;
                    case ';':
                        getNextToken(); // ignore top-level semicolons
                        break;
                    default:
                        HandleTopLevelExpression();
                        break;
                }
                break;
        }
    }
}


//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; 
  BinopPrecedence['/'] = 40;  
  BinopPrecedence['%'] = 40; 

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

auto &JD = TheJIT->getMainJITDylib();
auto DL = TheJIT->getDataLayout();
llvm::orc::MangleAndInterner Mangle(JD.getExecutionSession(), DL);

llvm::orc::SymbolMap Symbols;

Symbols[Mangle("printd")] = llvm::orc::ExecutorSymbolDef{
    llvm::orc::ExecutorAddr{llvm::pointerToJITTargetAddress(&printd)},
    llvm::JITSymbolFlags::Exported
};

Symbols[Mangle("sin")] = llvm::orc::ExecutorSymbolDef{
    llvm::orc::ExecutorAddr{
        llvm::pointerToJITTargetAddress(static_cast<double(*)(double)>(&sin))
    },
    llvm::JITSymbolFlags::Exported
};

ExitOnErr(JD.define(llvm::orc::absoluteSymbols(Symbols)));




  InitializeModuleAndManagers();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}
