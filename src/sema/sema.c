#include <stdbool.h>

#include "analyser.h"
#include "sema.h"

#include "syntax/ast/ast.h"

Analyser *init_analyser(Analyser *a, Source *src, AstNode *root) {
    *a = (Analyser){0};
    a->src = src;
    a->root = root;
    return a;
}

bool analyse(Analyser *a) {
    analyser_pass_0(a);
    return true;
}

void free_analyser(Analyser *a) { (void)a; };
