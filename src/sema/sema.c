#include <stdbool.h>

#include "analyser.h"
#include "sema.h"

#include "core/mem/mem.h"
#include "syntax/ast/ast.h"

static Analyser *init_analyser(Analyser *a, Source *src, AstNode *root, Arena *arena) {
    *a = (Analyser){0};
    a->src = src;
    a->root = root;
    a->info = arena;
    return a;
}

bool analyse(Source *src, AstNode *root, Arena *arena) {
    Analyser analyser = {0};
    Analyser *a = init_analyser(&analyser, src, root, arena);
    analyser_pass_0(a);
    return true;
}
