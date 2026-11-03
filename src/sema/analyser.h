#include <stdbool.h>

#include "sema.h"

struct Analyser {
    Source *src;
    AstNode *root;

    Arena *info;

    AstNode *cur_fn;
    size_t fn_depth;
    size_t loop_depth;
    size_t scope_depth;
};

bool analyser_pass_0(Analyser *a);
