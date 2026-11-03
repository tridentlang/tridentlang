#include "core/diags/diags.h"
#include "core/mem/mem.h"
#include "core/source/source.h"
#include "sema/sema.h"
#include "syntax/ast/ast.h"
#include "syntax/lexer/lexer.h"
#include "syntax/parser/parser.h"
#include "syntax/token/token.h"

int main(int argc, const char *const *argv) {
    if (argc != 2) {
        fatal("incorrect usage! \n");
        fatal("Correct usage : %s <file_pat>", argv[0]);
    }

    Source source = {0};
    Source *src = init_source(&source, argv[1]);

    Arena *tokens = init_arena(sizeof(Token) * 1024);
    Token *root_token = lex(src, tokens);

    Arena *nodes = init_arena(sizeof(Token) * 1024);
    AstNode *root_node = parse(src, root_token, nodes);
    free_arena(tokens);

    Analyser analyser = {0};
    Analyser *a = init_analyser(&analyser, src, root_node);
    analyse(a);
    free_analyser(a);

    free_source(src);
    free_arena(nodes);

    return 0;
}
