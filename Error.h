#ifndef __ERROR_H__
#define __ERROR_H__
#include "AST.h"
#include "AllInclude.h"

//.logError* - these are little helper functions for error handling
std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);
Value *LogErrorV(const char *Str);

#endif