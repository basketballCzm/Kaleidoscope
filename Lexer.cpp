//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//
//return the next token fron standard input
#include "Lexer.h"
#include "Error.h"
static int gettok() {
    static int LastChar = ' ';
    //skip any whitespace
    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    //identifier: [a-zA-z][a-zA_Z0-9]*
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar()))) {
            IdentifierStr += LastChar;
        }

        if ("def" == IdentifierStr) {
            return tok_def;
        } else if ("extern" == IdentifierStr) {
            return tok_extern;
        }
        return tok_identifier;
    }

    //Number: [0-9.]+
    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    //Comment until end of line, ignore comment
    if ('#' == LastChar) {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && '\n' != LastChar && '\r' != LastChar);

        //ignore comment
        if (EOF != LastChar) {
            return gettok();
        }
    }

    //check for end of file, Do not eat the EOF
    if (LastChar == EOF) {
        return tok_eof;
    }

    int ThisChar = LastChar;
    LastChar     = getchar();
    return ThisChar;
}