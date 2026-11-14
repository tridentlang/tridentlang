#include <stdbool.h>
#include <stdio.h>

#include "analyser.h"
#include "sema.h"

#include "syntax/ast/ast.h"

static bool enter_function(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

static bool leave_function(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

static bool enter_if_stmt(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

static bool leave_if_stmt(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

static bool enter_else_stmt(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

static bool leave_else_stmt(Visitor *v, AstNode *node) {
    (void)node;
    (void)v;
    return true;
}

bool analyser_pass_1(Analyser *a) {
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

    print_ast(a->root);
    fprintf(stderr, "\nsema-pass-1\n");
    ast_walk(&v, a->root);
    return true;
}
