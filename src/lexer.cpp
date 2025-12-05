#include "../include/lexer.h"
#include "../include/token.h"

using namespace std;

int CurrentToken;
string IdentifierStr;
double NumVal;

// Gets one character at a time using getchar() and store in LastChar
int gettok() {
  static int LastChar = ' ';

  // Ignore Whitespaces between Tokens
  while (isspace(LastChar)) {
    LastChar = getchar();
  }

  // Recognize Identifier and Specific Keywords
  // isalpha(char) returns True if the char is an alphabet, else return False
  if (isalpha(LastChar)) {
    IdentifierStr = LastChar;

    // Stacking together all alphanumeric characters into IdentifierStr
    // isalnum(char) returns True if the char is an alphanumeric, else return False
    while (isalnum(LastChar = getchar())) {
      IdentifierStr += LastChar;
    }

    if (IdentifierStr == "def") {
      return tok_def;
    }

    if (IdentifierStr == "extern") {
      return tok_extern;
    }

    return tok_identifier;
  }

  // Stacking together only numeric values
  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;

    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    // Convert numeric string to numeric value
    // that we are store in NumVal
    NumVal = strtod(NumStr.c_str(), 0);
    return tok_number;
  }

  // Handling comments by skipping to the end of the line
  // and return the next token
  if (LastChar == '#') {
    do {
      LastChar = getchar();
    } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF) {
      return gettok();
    }
  }

  // Finally, if the input doesn't match one of the above cases
  // it's either an operator character like '+' or the end of the file
  if (LastChar == EOF) {
    return tok_eof;
  }

  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

int getNextToken() {
  return CurTok = gettok();
}

// Note to self: the getchar() is getting the characters from the input stram as we run it