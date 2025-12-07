#include "../include/VariableExprAST.h"

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = NamedValues[Name];
    if (!V) {
        // If variable not found, create a dummy alloca in entry block
        // V = Builder->CreateAlloca(llvm::Type::getDoubleTy(*TheContext), nullptr, Name);
        // NamedValues[Name] = V;

        return LogErrorV("Unknown variable name");
    }
    // return Builder->CreateLoad(llvm::Type::getDoubleTy(*TheContext), V, Name);
    return V;
}