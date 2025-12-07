#include "../include/FunctionAST.h"

// Generates LLVM code for functions declarations
llvm::Function *FunctionAST::codegen() {
  llvm::Function *TheFunction = TheModule->getFunction(Proto->getName());

  if (!TheFunction) {
        // Generate function prototype
        TheFunction = Proto->codegen();
        if (!TheFunction) return nullptr;
  }

  // if (!TheFunction) {
  //   TheFunction = Proto->codegen();
  // }

  // if (!TheFunction) {
  //   return nullptr;
  // }
  
  if (!TheFunction->empty()){
    return llvm::cast<llvm::Function>(LogErrorV("Function cannot be redefined."));
  }


  llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);
  NamedValues.clear();
  for (auto &Arg : TheFunction->args()) {
    NamedValues[std::string(Arg.getName())] = &Arg;
  }

  if (llvm::Value *RetVal = Body->codegen()) {
    Builder->CreateRet(RetVal);
    verifyFunction(*TheFunction);
    return TheFunction;
  }

  TheFunction->eraseFromParent();
  return nullptr;
}