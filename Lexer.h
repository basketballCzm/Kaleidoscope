#ifndef __LEXER_H__
#define __LEXER_H__
//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//
#include "AllInclude.h"
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

static int gettok();

#endif