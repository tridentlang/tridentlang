#include <stdbool.h>

#include "core/diags/diags.h"
#include "sema.h"

#include "syntax/ast/ast.h"

static bool enter_function(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    if (a->fn_depth > 0)
        diag_err(a->src, node->span, true, "nested functions are not allowed.");
    a->fn_depth++;
    a->scope_depth++;
    return true;
}

static bool leave_function(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->fn_depth--;
    a->scope_depth--;
    return true;
}

static bool enter_if_stmt(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    if (a->scope_depth > 0)
        diag_err(a->src, node->span, true, "top level if statements are not allowed.");
    a->scope_depth++;
    return true;
}

static bool leave_if_stmt(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->scope_depth--;
    return true;
}

static bool enter_else_stmt(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->scope_depth++;
    return true;
}

static bool leave_else_stmt(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->scope_depth--;
    return true;
}

bool analyser_pass_0(Analyser *a) {
    Visitor v = {0};
    v.ctx = a;

    v.enter[AST_FUNCTION] = enter_function;
    v.leave[AST_FUNCTION] = leave_function;

    v.enter[AST_IF] = enter_if_stmt;
    v.leave[AST_IF] = leave_if_stmt;

    v.enter[AST_ELSE_IF] = enter_else_stmt;
    v.leave[AST_ELSE_IF] = leave_else_stmt;
    v.enter[AST_ELSE] = enter_else_stmt;
    v.leave[AST_ELSE] = leave_else_stmt;

    ast_walk(&v, a->root);
    return true;
}
