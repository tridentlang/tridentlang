#include <stdbool.h>

#include "analyser.h"
#include "sema.h"

#include "syntax/ast/ast.h"

Analser *init_analyser(Analser *a, Source *src, AstNode *root) {
    *a = (Analser){0};
    a->src = src;
    a->root = root;
    return a;
}

bool analyse(Analser *a) {
    analyser_pass_0(a);
    return true;
}

void free_analser(Analser *a) { (void)a; };
