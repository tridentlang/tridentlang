#pragma once

#include "syntax/ast/ast.h"
#include <stddef.h>

typedef struct {
    AstNode *root;
    bool in_func;
    AstNode *cur_fn;
    size_t loop_depth;
    size_t scope_depth;
} Analser;

Analser *init_analyser(Analser *a, AstNode *node);
bool analyse(Analser *a);
void free_analser(Analser *a);
