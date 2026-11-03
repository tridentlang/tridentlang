#pragma once

#include "core/mem/mem.h"
#include "core/source/source.h"
#include "syntax/token/token.h"

typedef struct Lexer Lexer;

Token *lex(Source *src, Arena *arena);
