#pragma once

#include "core/mem/mem.h"
#include "core/source/source.h"
#include "syntax/ast/ast.h"

typedef struct Analyser Analyser;

#define ANALYSER(v) ((Analyser *)(v)->ctx)

bool analyse(Source *src, AstNode *root, Arena *arena);
