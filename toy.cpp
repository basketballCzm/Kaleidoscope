#include "AST.h"
#include "Codegen.h"
#include "Error.h"
#include "Lexer.h"
#include "Parser.h"
//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void
HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top top-level expression inti a anonymous function
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        //skip token for error recovery
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
            case ';':  //ignore top-level semicolons
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

    //Make the module, which holds all the code.
    TheModule = llvm::make_unique<Module>("my first coder", TheContext);

    //Run the main "interpreter loop" now.
    MainLoop();

    // print out all of the generated code
    TheModule->print(errs(), nullptr);

    return 0;
}
