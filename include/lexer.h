#ifndef LEXER_H
#define LEXER_H
#include <cstdlib>
#include <string>

using namespace std;

extern int CurrentToken;
// Token Buffer
int gettok();
// Updates Current Token with the next Token
int getNextToken();

extern string IdentifierStr;
extern double NumVal;

#endif // LEXER_H