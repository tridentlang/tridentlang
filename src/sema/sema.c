#include <stdbool.h>
#include <stdlib.h>

#include "sema.h"

#include "core/mem/mem.h"
#include "syntax/ast/ast.h"

struct Analser {
    AstNode *root;
};

Analser *init_analyser(AstNode *root) {
    Analser *a = xmalloc(sizeof(Analser));
    a->root = root;
    return a;
}

bool analyse(Analser *a) {
    print_ast(a->root);
    putc('\n', stderr);
    return true;
}

void free_analser(Analser *a) { free(a); };
