// #include "../include/kaleidoscope.h"


// // This is an object that owns LLVM core data structures
// llvm::LLVMContext TheContext;

// // This is a helper object that makes easy to generate LLVM instructions
// llvm::IRBuilder<> Builder(TheContext);

// // This is an LLVM construct that contains functions and global variables
// std::unique_ptr<llvm::Module> TheModule = 
//     std::make_unique<llvm::Module>("msk2", TheContext);

// // This map keeps track of which values are defined in the current scope
// std::map<std::string, llvm::Value *> NamedValues;

#include "../include/kaleidoscope.h"
#include "llvm/IR/Constants.h"

// ---------- Global Objects ----------
std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> TheModule;

std::unique_ptr<KaleidoscopeJIT> TheJIT;

std::map<std::string, llvm::Value *> NamedValues;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

// ---------- Error Helper ----------
llvm::Value *LogErrorV(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

// ---------- Function Lookup ----------
llvm::Function *getFunction(const std::string &Name) {
  // Is it in the current module?
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // Is it a known prototype?
  auto It = FunctionProtos.find(Name);
  if (It != FunctionProtos.end())
    return It->second->codegen();

  return nullptr;
}

// ---------- Module + Pass Manager Setup ----------
void InitializeModuleAndManagers() {
  TheContext = std::make_unique<llvm::LLVMContext>();
  Builder    = std::make_unique<llvm::IRBuilder<>>(*TheContext);
  TheModule  = std::make_unique<llvm::Module>("KaleidoscopeJIT", *TheContext);

  // The JIT decides the data layout
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // (Optional) You can add optimization passes here if needed later
}
