#include <stdbool.h>

#include "analyser.h"
#include "sema.h"

#include "syntax/ast/ast.h"

Analser *init_analyser(Analser *a, AstNode *root) {
    *a = (Analser){0};
    a->root = root;
    return a;
}

bool analyse(Analser *a) {
    analyser_pass_1(a);
    return true;
}

void free_analser(Analser *a) { (void)a; };
