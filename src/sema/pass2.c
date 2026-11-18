#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "analyser.h"
#include "core/diags/diags.h"
#include "core/hashmap/hashmap.h"
#include "core/sv/sv.h"
#include "sema.h"
#include "syntax/ast/ast.h"
#include "type/type.h"

bool enter_type_name(Visitor *v, AstNode *node) {
    (void)v;
    Analyser *a = ANALYSER(v);

    node->as.type_tok.type = (size_t)get_from_hash_map(
        a->type_table, node->as.ident.name.str, node->as.ident.name.len);

    if (!node->as.type_tok.type)
        diag_info(a->src, node->span, true,
                  "void or complex types are not yet supported!!");

    return true;
}

bool analyser_pass_2(Analyser *a) {
    Visitor v = {0};
    v.ctx = a;

    a->type_pool = init_type_pool();
    a->type_table = init_hash_map((size_t)PRIMITIVE_NONE * 2);

    for (size_t i = 0; i < (size_t)PRIMITIVE_NONE; i++) {
        const char *type = primitive_name((PrimitiveKind)i);
        size_t size = strlen(type);
        put_to_hash_map(a->type_table, type, size, (void *)i);
    }

    v.enter[AST_TYPE_NAME] = enter_type_name;

    ast_walk(&v, a->root);

    free_type_pool(a->type_pool);
    free_hash_map(a->type_table);

    return true;
}
