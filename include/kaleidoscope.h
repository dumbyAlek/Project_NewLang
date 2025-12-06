// #ifndef KALEIDOSCOPE_H
// #define KALEIDOSCOPE_H

// #include "llvm/ADT/APFloat.h"
// #include "llvm/ADT/STLExtras.h"
// #include "llvm/IR/BasicBlock.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/IR/DerivedTypes.h"
// #include "llvm/IR/Function.h"
// #include "llvm/IR/IRBuilder.h"
// #include "llvm/IR/LLVMContext.h"
// #include "llvm/IR/Module.h"
// #include "llvm/IR/Type.h"
// #include "llvm/IR/Verifier.h"

// #include <map>
// #include <memory>
// #include <string>

// // This is an object that owns LLVM core data structures
// extern llvm::LLVMContext TheContext;

// // This is a helper object that makes easy to generate LLVM instructions
// extern llvm::IRBuilder<> Builder;

// // This is an LLVM construct that contains functions and global variables
// extern std::unique_ptr<llvm::Module> TheModule;

// // This map keeps track of which values are defined in the current scope
// extern std::map<std::string, llvm::Value *> NamedValues;

// #endif // KALEIDOSCOPE_H

#ifndef KALEIDOSCOPE_H
#define KALEIDOSCOPE_H

#include <map>
#include <memory>
#include <string>

#include "KaleidoscopeJIT.h"

// LLVM Core
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

// LLVM Passes
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassManager.h"

// Global LLVM state (extern, defined in .cpp)
extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> TheModule;

extern std::unique_ptr<KaleidoscopeJIT> TheJIT;

// Values in current function scope
extern std::map<std::string, llvm::Value *> NamedValues;

// Function prototypes (for extern + forward-declared functions)
class PrototypeAST;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

// Utilities
llvm::Value *LogErrorV(const char *Str);
llvm::Function *getFunction(const std::string &Name);

// Module/Pass initializers
void InitializeModuleAndManagers();

#endif // KALEIDOSCOPE_H
