#include <stdbool.h>

#include "sema.h"

bool analyser_pass_1(Analser *a) {
    print_ast(a->root);
    putc('\n', stderr);
    return true;
}
