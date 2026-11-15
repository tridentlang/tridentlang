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

Info *scope_stack_lookup(Stack *scope_stack, StrView name) {
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
    a->current_scope_table = fn_scope_table;
    a->cur_fn = node;
    return true;
}

static bool leave_function(Visitor *v, AstNode *node) {
    (void)node;
    Analyser *a = ANALYSER(v);
    a->cur_fn = NULL;
    a->current_scope_table = stack_top(a->scope_stack);
    free_hash_map(stack_pop(a->scope_stack));
    return true;
}

static bool enter_param(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    Info *var = scope_stack_lookup(a->scope_stack, node->as.param.name);
    if (!var) {
        var = info(a->info, PARAM_VAR, 0, 0, (Value){0});
        var->decl = node;
    } else {
        diag_err(a->src, node->span, true,
                 "duplicate parameter name in a function definition.");
        diag_info(a->src, var->decl->span, false, "previous declaration at:");
    }
    if (var) node->as.param.info = var;
    put_to_hash_map(a->current_scope_table, node->as.param.name.str,
                    node->as.param.name.len, var);
    return true;
}

static bool enter_let(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    Info *var = scope_stack_lookup(a->scope_stack, node->as.let.name);
    if (!var) {
        var = info(a->info, PARAM_VAR, 0, 0, (Value){0});
        var->decl = node;
    } else {
        diag_err(a->src, node->span, true,
                 "declaration shadows previous declaration of `" SV_FMT "'.",
                 SV_ARG(node->as.let.name));
        diag_info(a->src, var->decl->span, false,
                  "previous declaration of `" SV_FMT "' at:", SV_ARG(node->as.let.name));
    }
    if (var) node->as.let.info = var;
    put_to_hash_map(a->current_scope_table, node->as.let.name.str, node->as.let.name.len,
                    var);
    return false;
}

static bool enter_identifier(Visitor *v, AstNode *node) {
    Analyser *a = ANALYSER(v);
    Info *var = scope_stack_lookup(a->scope_stack, node->as.ident.name);
    if (!var) {
        diag_err(a->src, node->span, true, "undefined identifier `" SV_FMT "'.",
                 SV_ARG(node->as.ident.name));
    }
    if (var) node->as.ident.info = var;
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

    v.enter[AST_PARAM] = enter_param;
    v.enter[AST_LET] = enter_let;
    v.enter[AST_IDENT] = enter_identifier;

    HashMap *global_scope = init_hash_map(16);
    stack_push(a->scope_stack, global_scope);

    // print_ast(a->root);
    // fprintf(stderr, "\nsema-pass-1\n\n");
    ast_walk(&v, a->root);

    free_hash_map(global_scope);

    free_stack(a->scope_stack);
    a->scope_stack = NULL;
    return true;
}
