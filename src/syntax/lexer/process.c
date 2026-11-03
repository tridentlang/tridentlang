#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "core/sv/sv.h"
#include "intrinsics.h"
#include "process.h"

#include "core/diags/diags.h"
#include "core/mem/mem.h"
#include "syntax/token/token.h"

char peek(const Lexer *l, size_t offset) {
    if (l->i + offset >= l->buf_len) {
        return '\0';
    }
    return l->buffer[l->i + offset];
}

bool starts_number(Lexer *l) {
    const char c = peek(l, 0);
    return is_digit(c) || (c == '.' && is_digit(peek(l, 1)));
}

void consume(Lexer *l, size_t count) {
    for (size_t k = 0; k < count && l->i < l->buf_len; ++k) {
        if (l->buffer[l->i] == '\n') {
            ++l->ln;
            l->cn = 1;
        } else {
            ++l->cn;
        }
        ++l->i;
    }
}

size_t consume_while(Lexer *l, bool (*pred)(char)) {
    size_t count = 0;
    while (!is_at_end(l) && pred(peek(l, 0))) {
        consume(l, 1);
        ++count;
    }
    return count;
}

void lex_intrinsic(Lexer *l, Location start) {
    const size_t start_i = l->i;
    consume_while(l, is_ident_char);
    const size_t length = l->i - start_i;
    StrView word = sv_at(l->buffer, start_i, l->i - start_i);
    const struct Intrinsic *intrinsic = get_intrinsic_kind(word.str, length);

    const Location end = location(l->ln, l->cn, l->i);

    if (intrinsic == NULL) {
        lexer_error(l, start, "unknown intrensic `@" SV_FMT "'.", SV_ARG(word));
        lexer_emit(l, start, end, UNKNOWN_TOKEN, word, (Value){0});
        return;
    }
    TokenKind kind = intrinsic->kind;

    if (kind == IC_HERE) {
        const char *file_path = l->src->file_path;
        size_t str_len = strlen(file_path) + 22;
        char *string = arena_alloc(l->token, strlen(file_path) + 22);
        sprintf(string, "%s:%zu:%zu", file_path, start.ln, start.cn);
        word.str = string;
        word.len = str_len;
        kind = STRING_LIT;
    }

    lexer_emit(l, start, end, kind, word, (Value){0});
}

void lexer_emit(Lexer *l, Location start, Location end, TokenKind kind, StrView lexeme,
                Value val) {
    Span s = span(start, end);
    Token *new_token = arena_alloc(l->token, sizeof(Token));
    *new_token = token(kind, lexeme, s, val);
    Token *last_token = l->token_tail;
    new_token->last = last_token;
    l->token_tail->next = new_token;
    l->token_tail = new_token;
}

void lexer_error(Lexer *l, Location start, const char *fmt, ...) {
    ++l->errorc;
    fprintf(stderr, "%s:%zu:%zu: ", l->src->file_path, start.ln, start.cn);
    va_list args;
    va_start(args, fmt);
    verror(fmt, args);
    va_end(args);
}
