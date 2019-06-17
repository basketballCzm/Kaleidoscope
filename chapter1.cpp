#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "llvm/ADT/STLExtras.h"

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
    tok_eof = -1,

    //commands
    tok_def    = -2,
    tok_extern = -3,

    //primary
    tok_identifier = -4,
    tok_number     = -5,
};

//filled in if tok_identifier
static std::string IdentifierStr;
//filled in if tok_number
static double NumVal;

//return the next token fron standard input
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

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
//expr  prototype   function object
//this can parser expr and functionbody

//ExprAST - Base class for all expression nodes
class ExprAST {
   public:
    virtual ~ExprAST() {}
};

//NumberExprAST - Expression class for numeric literals like "1.0"
class NumberExprAST : public ExprAST {
    double Val;

   public:
    NumberExprAST(double Val) : Val(Val) {}
};

//VariableExprAST - Expression class for referencing a variable , like "a"
class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
};

//BinaryExprAST - Expression class for a binary operator
//Note that there is no discussion about precedence of binary operators, lexical structure, etc.
class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

   public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

//callExprAST - Expression class for function calls
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

   public:
    PrototypeAST(const std::string &name, std::vector<std::string> Args)
        : Name(name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }
};

//FunctionAST - this class represents a function definition itself
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

   public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}
static std::unique_ptr<ExprAST> ParsePrimary();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

//.logError* - these are little helper functions for error handling
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

///numberExpr ::= number;  token is tok_number,it will create NumberExprAST node.
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = llvm::make_unique<NumberExprAST>(NumVal);
    getNextToken();  // consumber the number
    return std::move(Result);
}

/// expression ::= primary binoprhs
///  an expression is a primary expression potentially followed by a sequence of [binop,primaryexpr] pairs:
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(LHS));
}

//parenexpr := '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();  //eat '('
    auto V = ParseExpression();
    if (!V) {
        return nullptr;
    }

    if (CurTok != ')') {
        return LogError("expected ')'");
    }
    getNextToken();  //eat ')'
    return V;
}

/// identifierexpr
/// ::= identifier
/// ::= identifier '(' expression* ')'
/// It handles this by checking to see if the token after the identifier is a ‘(‘ token, constructing either a VariableExprAST or CallExprAST node as appropriate.
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    getNextToken();       //eat identifier
    if ('(' != CurTok) {  // simple variable ref
        return llvm::make_unique<VariableExprAST>(IdName);
    }

    /// call function
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (')' != CurTok) {
        while (1) {
            if (auto Arg = ParseExpression()) {
                Args.push_back(std::move(Arg));
            } else {
                return nullptr;
            }

            if (')' == CurTok) {
                break;
            }
            //func(a, b, c)
            if (',' != CurTok) {
                return LogError("Expected ')' or ',' in argument list");
            }
            getNextToken();
        }
    }
    getNextToken();  //Eat the ')'

    return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
/// ::= identifierexpr
/// ::= numberexpr
/// ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("unknow token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

///BinaryPrecedence - This holds the precedence for each binary operator that is defined. if not binary operator return -1. the expression “a+b+(c+d)*e*f+g”. Operator precedence parsing considers this as a stream of primary expressions separated by binary operators. As such, it will first parse the leading primary expression “a”, then it will see the pairs [+, b] [+, (c+d)] [*, e] [*, f] and [+, g].
static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
    if (!isascii(CurTok)) {
        return -1;
    }

    //Make sure it is a declared binop
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) {
        return -1;
    }

    return TokPrec;
}

/// binoprhs ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    // if this is a binop, find its precedence.
    while (1) {
        //CurTok is a binary operator
        int TokPrec = GetTokPrecedence();

        // if this is a binop that binds at lease as tightly as the current binop consume it , otherwise we are done
        if (TokPrec < ExprPrec) {
            return LHS;
        }

        // Okay, we know this is a binop
        int BinOp = CurTok;
        getNextToken();  //eat binop

        // parse the primary expression after the binary operator
        // （a + b）binop unparsed ; a + ( b binop unparsed)
        auto RHS = ParsePrimary();
        if (!RHS) {
            return nullptr;
        }

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            //we recursively invoke the ParseBinOpRHS function specifying “TokPrec+1” as the minimum precedence required for it to continue.
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS) {
                return nullptr;
            }
        }
        //Merge LHS/RHS
        LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }  // loop around to the top of the while loop
}

/// prototype ::= id '(' id* ')' ;The next thing missing is handling of function prototypes.
/// function prototypes : id(id id id)
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (tok_identifier != CurTok) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string FnName = IdentifierStr;
    getNextToken();

    if ('(' != CurTok) {
        return LogErrorP("Expected '(' in prototype");
    }

    //read the list of argument names.
    std::vector<std::string> ArgNames;
    while (tok_identifier == getNextToken()) {
        ArgNames.push_back(IdentifierStr);
    }
    if (')' != CurTok) {
        return LogErrorP("Expected ')' in prototype");
    }

    //success
    getNextToken();  //eat ')'

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

//definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();  //eat def
    auto Proto = ParsePrototype();
    if (!Proto) {
        return nullptr;
    }

    if (auto E = ParseExpression()) {
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/// In addition, we support ‘extern’ to declare functions like ‘sin’ and ‘cos’ as well as to support forward declaration of user functions. These ‘extern’s are just prototypes with no body:
static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();  //eat extern
    return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        //make an anonymous proto
        auto Proto = llvm::make_unique<PrototypeAST>("", std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition. \n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern \n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top top-level expression inti a anonymous function
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

/// top ::= definition | external | expression | ;
static void MainLoop() {
    while (1) {
        fprintf(stderr, "ready>");
        switch (CurTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
    // Install standard binary operators
    // 1 is lowest precedence
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;  //hihest

    //prime the first token
    fprintf(stderr, "ready> ");
    getNextToken();

    //Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}
