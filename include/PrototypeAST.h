#ifndef PROTOTYPE_AST_H
#define PROTOTYPE_AST_H

#include "ExprAST.h"
#include <vector>
#include <string>
#include "llvm_all.h"
#include "kaleidoscope.h"

using namespace std;

// Represents the "prototype" for a function,
// which captures its name, and its argument names
class PrototypeAST {
  string Name;
  vector<string> Args;

public:
  PrototypeAST(const string &name, vector<string> Args) 
  : Name(name), Args(move(Args)) {}

  
  const string &getName() const { return Name; }
  void setName(const std::string &NewName) { 
        Name = NewName; 
  }
  
   llvm::Function *codegen();
};

#endif // PROTOTYPE_AST_H