#ifndef __PARSER_H__
#define __PARSER_H__
#include "AST.h"
#include "AllInclude.h"
#include "Lexer.h"
/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken();

///BinaryPrecedence - This holds the precedence for each binary operator that is defined. if not binary operator return -1. the expression “a+b+(c+d)*e*f+g”. Operator precedence parsing considers this as a stream of primary expressions separated by binary operators. As such, it will first parse the leading primary expression “a”, then it will see the pairs [+, b] [+, (c+d)] [*, e] [*, f] and [+, g].
static std::map<char, int> BinopPrecedence;

///numberExpr ::= number;  token is tok_number,it will create NumberExprAST node.
static std::unique_ptr<ExprAST> ParseNumberExpr();

/// expression ::= primary binoprhs
///  an expression is a primary expression potentially followed by a sequence of [binop,primaryexpr] pairs:
static std::unique_ptr<ExprAST> ParseExpression();

//parenexpr := '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr();

/// identifierexpr
/// ::= identifier
/// ::= identifier '(' expression* ')'
/// It handles this by checking to see if the token after the identifier is a ‘(‘ token, constructing either a VariableExprAST or CallExprAST node as appropriate.
static std::unique_ptr<ExprAST> ParseIdentifierExpr();

/// primary
/// ::= identifierexpr
/// ::= numberexpr
/// ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary();

static int GetTokPrecedence();

/// binoprhs ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

/// prototype ::= id '(' id* ')' ;The next thing missing is handling of function prototypes.
/// function prototypes : id(id id id)
static std::unique_ptr<PrototypeAST> ParsePrototype();

//definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition();

/// In addition, we support ‘extern’ to declare functions like ‘sin’ and ‘cos’ as well as to support forward declaration of user functions. These ‘extern’s are just prototypes with no body:
static std::unique_ptr<PrototypeAST> ParseExtern();

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr();

#endif