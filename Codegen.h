#ifndef __CODEGEN_H__
#define __CODEGEN_H__
#include "AllInclude.h"

static LLVMContext TheContext;
static IRBuilder<> Builder;
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

#endif