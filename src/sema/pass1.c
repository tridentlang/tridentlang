#include <stdbool.h>
#include <stddef.h>

#include "analyser.h"
#include "sema.h"

#include "core/diags/diags.h"
#include "core/hashmap/hashmap.h"
#include "core/mem/mem.h"
#include "core/sv/sv.h"
#include "syntax/ast/ast.h"
#include "syntax/token/token.h"
#include "type/type.h"

static Info *scope_stack_lookup(Stack *scope_stack, StrView name) {
    if (scope_stack == NULL || scope_stack->size == 0) {
        return NULL;
    }
    size_t i = scope_stack->size;
    while (i > 0) {
        i--;
        HashMap *curr_scope = (HashMap *)scope_stack->data[i];

        if (curr_scope != NULL && has_in_hash_map(curr_scope, name.str, name.len)) {
            return (Info *)get_from_hash_map(curr_scope, name.str, name.len);
        }
    }
    return NULL;
}

static bool enter_function(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    HashMap *fn_scope_table = init_hash_map(16);
    stack_push(a->scope_stack, fn_scope_table);
    a->current_scope_table = stack_top(a->scope_stack);
    a->cur_fn = node;
    return true;
}

static bool leave_function(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->cur_fn = NULL;
    HashMap *popped = stack_pop(a->scope_stack);
    a->current_scope_table = stack_top(a->scope_stack);
    free_hash_map(popped);
    return true;
}

static bool enter_block(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    HashMap *scope_table = init_hash_map(16);
    stack_push(a->scope_stack, scope_table);
    a->current_scope_table = stack_top(a->scope_stack);
    return true;
}

static bool leave_block(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    HashMap *popped = stack_pop(a->scope_stack);
    a->current_scope_table = stack_top(a->scope_stack);
    free_hash_map(popped);
    return true;
}

static bool enter_param(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    StrView name = node->as.param.name;
    Info *prev = NULL;
    if (has_in_hash_map(a->current_scope_table, name.str, name.len)) {
        prev = (Info *)get_from_hash_map(a->current_scope_table, name.str, name.len);
    }
    if (prev) {
        diag_err(a->src, node->span, true,
                 "duplicate parameter name in a function definition.");
        diag_info(a->src, prev->decl->span, false, "previous declaration at:");
    }
    Info *var = info(a->info, PARAM_VAR, 0, 0, (Value){0});
    var->decl = node;
    node->as.param.info = var;
    put_to_hash_map(a->current_scope_table, name.str, name.len, var);
    return true;
}

static bool enter_let(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    StrView name = node->as.let.name;

    if (node->as.let.expr) {
        ast_walk(v, node->as.let.expr);
    }

    Info *prev = scope_stack_lookup(a->scope_stack, name);
    if (prev) {
        diag_err(a->src, node->span, true,
                 "declaration shadows previous declaration of `" SV_FMT "'.",
                 SV_ARG(name));
        diag_info(a->src, prev->decl->span, false,
                  "previous declaration of `" SV_FMT "' at:", SV_ARG(name));
    }
    Info *var = info(a->info, FRAME_VAR, 0, 0, (Value){0});
    var->decl = node;
    node->as.let.info = var;
    put_to_hash_map(a->current_scope_table, name.str, name.len, var);
    return false;
}

static bool enter_identifier(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    Info *var = scope_stack_lookup(a->scope_stack, node->as.ident.name);
    if (!var) {
        diag_err(a->src, node->span, true, "undefined identifier `" SV_FMT "'.",
                 SV_ARG(node->as.ident.name));
        return false;
    }
    node->as.ident.info = var;
    return false;
}

static bool enter_member(Visitor *v, AstNode *node) {
    ast_walk(v, node->as.member.left);
    return false;
}

bool analyser_pass_1(Analyser *a) {
    Stack scope_stack__ = {0};
    Stack *scope_stack = init_stack(&scope_stack__, 16);
    Visitor v = {0};
    a->scope_stack = scope_stack;
    v.ctx = a;

    v.enter[AST_FUNCTION] = enter_function;
    v.leave[AST_FUNCTION] = leave_function;

    v.enter[AST_BLOCK] = enter_block;
    v.leave[AST_BLOCK] = leave_block;

    v.enter[AST_PARAM] = enter_param;
    v.enter[AST_LET] = enter_let;
    v.enter[AST_IDENT] = enter_identifier;

    v.enter[AST_MEMBER] = enter_member;
    v.enter[AST_DREF_MEMBER] = enter_member;

    v.enter[AST_IF] = enter_block;
    v.leave[AST_IF] = leave_block;
    v.enter[AST_ELSE_IF] = enter_block;
    v.leave[AST_ELSE_IF] = leave_block;
    v.enter[AST_WHILE] = enter_block;
    v.leave[AST_WHILE] = leave_block;
    v.enter[AST_LOOP_WHILE] = enter_block;
    v.leave[AST_LOOP_WHILE] = leave_block;

    HashMap *global_scope = init_hash_map(16);
    stack_push(a->scope_stack, global_scope);
    a->current_scope_table = global_scope;

    // print_ast(a->root);
    // fprintf(stderr, "\nsema-pass-1\n\n");
    ast_walk(&v, a->root);

    free_hash_map(global_scope);

    free_stack(a->scope_stack);
    a->scope_stack = NULL;
    a->current_scope_table = NULL;
    return true;
}
