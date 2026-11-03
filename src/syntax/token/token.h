#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "core/span/span.h"
#include "core/sv/sv.h"

typedef enum {
#define LITERAL(tok, name) tok##_LIT,
#define KEYWORD(tok, name) KW_##tok,
#define PUNCTUATION(tok, name) PUNCT_##tok,
#define TOKEN(tok, name) tok,
#define INTRINSIC(tok, name) IC_##tok,
#include "token.def"
#undef INTRINSIC
#undef LITERAL
#undef KEYWORD
#undef PUNCTUATION
#undef TOKEN
} TokenKind;

typedef union {
    size_t i;
    double f;
    bool b;
} Value;

typedef struct Token {
    TokenKind kind;
    StrView lexeme;
    Span span;
    Value val;
    struct Token *last;
    struct Token *next;
} Token;

Token token(TokenKind kind, StrView lexeme, Span span, Value val);
const char *token_kind_to_str(TokenKind kind);
void print_tokens(const Token *tokens);
