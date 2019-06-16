#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token{
    tok_eof = -1,

    //commands
    tok_def = -2,
    tok_extern = -3,

    //primary
    tok_identifier = -4,
    tok_number = -5,
};

//filled in if tok_identifier
static std::string IdentifierStr;
//filled in if tok_number
static double NumVal;

//return the next token fron standard input
static int gettok(){
    static int LastChar = ' ';
    //skip any whitespace
    while(isspace(LastChar)){
        LastChar = getchar();
        //identifier: [a-zA-z][a-zA_Z0-9]*
        if(isalpha(LastChar)){
            IdentifierStr = LastChar;
            while(isalnum((LastChar = getchar()))){
                IdentifierStr += LastChar;
            }

            if("def" == IdentifierStr){
                return tok_def;
            }else if("extern" == IdentifierStr){
                return tok_extern;
            }
            return tok_identifier;
        }

        //Number: [0-9.]+
        if(isdigit(LastChar) || LastChar == '.'){
            std::string NumStr;
            do{
                NumStr += LastChar;
                LastChar = getchar()
            }while(isdigit(LastChar) || LastChar == '.');

            NumVal = strtod(NumStr.c_str(), 0);
            return tok_number;
        }

        //Comment until end of line, ignore comment
        if('#' == LastChar){
            do{
                LastChar = getchar();
            }while(LastChar != EOF && '\n' != LastChar && '\r' != LastChar);

            //ignore comment
            if(EOF != LastChar){
                return gettok();
            }
        }

        //check for end of file, Do not eat the EOF
        if(LastChar == EOF) return tok_eof;

        int ThisChar = LastChar;
        LastChar = getchar();
        return ThisChar;
    }
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
//expr type variable 

//ExprAST - Base class for all expression nodes
class ExprAST{
public:
    virtual ~ExprAST() {}
};
