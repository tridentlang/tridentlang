#pragma once

#include <stddef.h>

#include "core/source/source.h"
#include "syntax/ast/ast.h"

typedef struct {
    Source *src;
    AstNode *root;
    AstNode *cur_fn;
    size_t fn_depth;
    size_t loop_depth;
    size_t scope_depth;
} Analyser;

#define ANALYSER(v) ((Analyser *)(v)->ctx)

Analyser *init_analyser(Analyser *a, Source *src, AstNode *root);
bool analyse(Analyser *a);
void free_analyser(Analyser *a);
