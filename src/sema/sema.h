#pragma once

#include <stddef.h>

#include "core/source/source.h"
#include "syntax/ast/ast.h"

typedef struct {
    Source *src;
    AstNode *root;
    bool in_func;
    AstNode *cur_fn;
    size_t loop_depth;
    size_t scope_depth;
} Analser;

#define ANALYSER(v) ((Analser *)(v)->ctx)

Analser *init_analyser(Analser *a, Source *src, AstNode *root);
bool analyse(Analser *a);
void free_analser(Analser *a);
