#include <stdbool.h>

#include "ast.h"

static void kids_add(AstKids *k, AstNode *head) {
    if (head) k->heads[k->n++] = head;
}

AstKids ast_kids(const AstNode *n) {
    AstKids k = {0};
    switch (n->kind) {
    case AST_UNARY: kids_add(&k, n->as.unary.operand); break;

    case AST_BINARY:
        kids_add(&k, n->as.binary.lhs);
        kids_add(&k, n->as.binary.rhs);
        break;

    case AST_COMPOUND: kids_add(&k, n->as.compound.assignees); break;

    case AST_IF:
    case AST_ELSE_IF:
    case AST_WHILE:
    case AST_LOOP_WHILE:
        kids_add(&k, n->as.cond.cond);
        kids_add(&k, n->as.cond.then_b);
        kids_add(&k, n->as.cond.else_b);
        break;

    case AST_CASE:
        kids_add(&k, n->as.ctl.expr);
        kids_add(&k, n->as.ctl.body);
        break;

    case AST_INDEX:
        kids_add(&k, n->as.index.left);
        kids_add(&k, n->as.index.index);
        break;

    case AST_MEMBER:
    case AST_DREF_MEMBER:
        kids_add(&k, n->as.member.left);
        kids_add(&k, n->as.member.member);
        break;

    case AST_CALL: kids_add(&k, n->as.call.args); break;

    case AST_PROGRAM:
    case AST_BLOCK:
    case AST_EXPR_STMT:
    case AST_CONTINUE:
    case AST_BREAK:
    case AST_RETURN:
    case AST_ELSE:
    case AST_LOOP: kids_add(&k, n->as.stmt.expr); break;

    case AST_LET:
        kids_add(&k, n->as.let.type);
        kids_add(&k, n->as.let.expr);
        break;

    case AST_TYPE: kids_add(&k, n->as.type.type); break;

    case AST_TYPE_ARRAY:
        kids_add(&k, n->as.array.inner);
        kids_add(&k, n->as.array.len);
        break;

    case AST_TYPE_POINTER:
    case AST_TYPE_SLICE: kids_add(&k, n->as.pointer.inner); break;

    case AST_TYPE_FUNC:
        kids_add(&k, n->as.func_t.return_type);
        kids_add(&k, NULL);
        kids_add(&k, n->as.func_t.args);
        break;

    case AST_UNION:
    case AST_STRUCT: kids_add(&k, n->as.structure.body); break;

    case AST_FUNCTION:
        kids_add(&k, n->as.func.return_type);
        kids_add(&k, n->as.func.params);
        kids_add(&k, n->as.func.body);
        break;

    case AST_PARAM: kids_add(&k, n->as.param.type); break;

    default: break;
    }
    return k;
}

void ast_walk(Visitor *visitor, AstNode *node) {
    for (; node && !visitor->stop; node = node->next) {
        VisitFn enter_hook = visitor->enter[node->kind];
        bool descend = enter_hook ? enter_hook(visitor, node) : true;

        if (descend && !visitor->stop) {
            AstKids k = ast_kids(node);
            for (size_t i = 0; i < k.n; i++)
                ast_walk(visitor, k.heads[i]);
        }

        VisitFn lv = visitor->leave[node->kind];
        if (lv && !visitor->stop) lv(visitor, node);
    }
}
