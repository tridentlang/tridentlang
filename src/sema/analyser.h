#include <stdbool.h>

#include "sema.h"

#include "core/hashmap/hashmap.h"
#include "core/mem/mem.h"

struct Analyser {
    Source *src;
    AstNode *root;

    Arena *info;

    AstNode *cur_fn;
    size_t fn_depth;
    size_t loop_depth;

    size_t scope_depth;
    Stack *scope_stack;
    HashMap *current_scope_table;
};

bool analyser_pass_0(Analyser *a);
bool analyser_pass_1(Analyser *a);
