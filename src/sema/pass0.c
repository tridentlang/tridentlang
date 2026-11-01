#include <stdbool.h>

#include "sema.h"

#include "core/def.h"
#include "syntax/ast/ast.h"

static bool enter_function(Visitor *v, AstNode *node) {
    (void)v;
    const char *kind = ast_kind_to_string(node->kind);
    printf(FG_RED "%s: " FG_BLUE "%s" FG_GREEN "\"%s\"" FG_RED " %s" RESET, kind,
           node->as.func.public ? "public " : "", node->as.func.name,
           node->as.func.noreturn ? "noreturn" : "");
    putc('\n', stdout);
    return true;
}

bool analyser_pass_0(Analser *a) {
    Visitor v = {0};
    v.ctx = a;
    v.enter[AST_FUNCTION] = enter_function;
    ast_walk(&v, a->root);
    return true;
}
