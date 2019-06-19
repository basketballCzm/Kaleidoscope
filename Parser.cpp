//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//
#include "Parser.h"
#include "Error.h"

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int getNextToken() {
    return CurTok = gettok();
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