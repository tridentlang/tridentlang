#include <stdbool.h>

#include "core/diags/diags.h"
#include "sema.h"

#include "syntax/ast/ast.h"

static bool enter_function(Visitor *v, AstNode *node) {
    Analser *a = ANALYSER(v);
    if (a->scope_depth && a->in_func) {
        diag_err(
            a->src, node->span, true,
            "a func is a top level construct. a function inside of another is not allowed.");
    }
    a->in_func = true;
    a->scope_depth += 1;
    return true;
}

static bool leave_function(Visitor *v, AstNode *node) {
    (void)node;
    Analser *a = ANALYSER(v);
    a->in_func = false;
    a->scope_depth += 1;
    return true;
}

bool analyser_pass_0(Analser *a) {
    Visitor v = {0};
    v.ctx = a;

    v.enter[AST_FUNCTION] = enter_function;
    v.leave[AST_FUNCTION] = leave_function;

    ast_walk(&v, a->root);
    return true;
}
