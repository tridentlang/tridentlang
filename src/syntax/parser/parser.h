#pragma once

#include "core/source/source.h"
#include "syntax/ast/ast.h"
#include "syntax/token/token.h"

typedef struct Parser Parser;

AstNode *parse(Source *src, Token *tokens, Arena *arena);
