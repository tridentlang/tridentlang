#pragma once

#include "syntax/ast/ast.h"

typedef struct Analser Analser;

Analser *init_analyser(AstNode *node);
bool analyse(Analser *a);
void free_analser(Analser *a);
