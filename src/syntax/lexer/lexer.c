#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/sv/sv.h"
#include "keywords.h"
#include "lexer.h"
#include "process.h"

#include "core/diags/diags.h"
#include "core/mem/mem.h"
#include "core/source/source.h"
#include "syntax/token/token.h"

typedef struct {
    const char *text;
    size_t len;
    TokenKind kind;
} Punctuator;

static const Punctuator punctuators[] = {
#define LITERAL(tok, name)
#define KEYWORD(tok, name)
#define PUNCTUATION(tok, str) {str, sizeof(str) - 1, PUNCT_##tok},
#define TOKEN(tok, name)
#define INTRINSIC(tok, name)
// violation: this is so that Punctuator are easy to add and/or remove, through token.def
#include "syntax/token/token.def"
#undef INTRINSIC
#undef TOKEN
#undef PUNCTUATION
#undef KEYWORD
#undef LITERAL
};

static void lex_identifier(Lexer *l, Location start) {
    consume_while(l, is_ident_char);
    const size_t length = l->i - start.i;
    StrView word = sv_at(l->buffer, start.i, l->i - start.i);
    const struct Keyword *keyword = get_keyword_kind(word.str, length);
    const Location end = location(l->ln, l->cn, l->i);
    TokenKind kind = keyword != NULL ? keyword->kind : IDENTIFIER_LIT;
    lexer_emit(l, start, end, kind, word, (Value){0});
}

static bool consume_until_quote(Lexer *l, char quote) {
    while (!is_at_end(l) && peek(l, 0) != '\n') {
        const char c = peek(l, 0);
        if (c == quote) {
            return true;
        }
        if (c == '\\') {
            consume(l, 1);
            if (is_at_end(l) || peek(l, 0) == '\n') {
                return false;
            }
        }
        consume(l, 1);
    }
    return false;
}

static void lex_quoted(Lexer *l, Location start) {
    const char quote = peek(l, 0);
    consume(l, 1);
    const size_t body = l->i;

    if (!consume_until_quote(l, quote)) {
        const char *msg = quote == '"' ? "Unterminated string literal."
                                       : "Unterminated character constant.";
        fatal("%s:%zu:%zu: %s", l->src->file_path, start.ln, start.cn, msg);
        return;
    }

    StrView lexeme = sv_at(l->buffer, start.i, l->i - start.i);
    const size_t length = 1;
    decode_escapes(l, start, (char *)lexeme.str, l->i - body);
    consume(l, 1);

    if (quote == '"') {
        const Location end = location(l->ln, l->cn, l->i);
        lexer_emit(l, start, end, STRING_LIT, lexeme, (Value){0});
        return;
    }

    if (length == 0) lexer_error(l, start, "Empty character constant.");
    if (length > 2) lexer_error(l, start, "Character constant too long.");
    const Location end = location(l->ln, l->cn, l->i);
    lexer_emit(l, start, end, CHARACTER_LIT, lexeme, (Value){0});
}

static void lex_quoted_ident(Lexer *l, Location start) {
    consume(l, 1);
    const size_t body = l->i;
    bool closed = false;

    while (true) {
        const char ch = peek(l, 0);
        if (ch == '"') {
            closed = true;
            break;
        }
        if (ch == '\0' || ch == '\n') break;
        if (ch == '\\' && peek(l, 1) != '\0' && peek(l, 1) != '\n') consume(l, 1);
        consume(l, 1);
    }

    StrView word = sv_at(l->buffer, start.i, l->i - start.i);
    const size_t n = decode_escapes(l, start, (char *)word.str, body);

    if (closed) {
        consume(l, 1);
        if (n == 0) lexer_error(l, start, "Quoted identifier is empty.");
    } else {
        lexer_error(l, start, "Unterminated quoted identifier.");
    }

    const Location end = location(l->ln, l->cn, l->i);
    lexer_emit(l, start, end, IDENTIFIER_LIT, word, (Value){0});
    return;
}

static bool lex_punctuator(Lexer *l, Location start) {
    const size_t count = sizeof(punctuators) / sizeof(punctuators[0]);
    const Punctuator *best = NULL;
    for (size_t k = 0; k < count; ++k) {
        const Punctuator *p = &punctuators[k];
        if (l->i + p->len > l->buf_len ||
            memcmp(l->buffer + l->i, p->text, p->len) != 0) {
            continue;
        }
        if (best == NULL || p->len > best->len) {
            best = p;
        }
    }
    if (best == NULL) {
        return false;
    }
    consume(l, best->len);
    StrView punct = sv_at(l->buffer, start.i, l->i - start.i);
    const Location end = location(l->ln, l->cn, l->i);
    lexer_emit(l, start, end, best->kind, punct, (Value){0});
    return true;
}

static Lexer *init_lexer(Lexer *l, Source *src, Arena *arena) {
    *l = (Lexer){0};
    l->src = src;
    l->buffer = src->buffer;
    l->buf_len = src->buffer_len;
    l->ln = 1;
    l->cn = 1;

    l->token = arena;

    l->token_head = arena_alloc(l->token, sizeof(Token));
    l->token_head->next = NULL;
    l->token_tail = l->token_head;

    return l;
}

Token *lex(Source *src, Arena *arena) {
    Lexer lexer = {0};
    Lexer *l = init_lexer(&lexer, src, arena);

    while (!is_at_end(l)) {
        const Location start = location(l->ln, l->cn, l->i);
        const char c = peek(l, 0);
        l->t_cn = start.cn;

        if (is_space(c)) {
            consume_while(l, is_space);
            continue;
        }
        if (c == '/' && peek(l, 1) == '/') {
            consume_while(l, is_comment);
            continue;
        }
        if (is_alpha(c) || c == '_') {
            lex_identifier(l, start);
            continue;
        }
        if (starts_number(l)) {
            lex_number(l, start);
            continue;
        }
        if (c == '"' || c == '\'') {
            lex_quoted(l, start);
            continue;
        }
        if (c == '@') {
            consume(l, 1);
            if (peek(l, 0) == '"') {
                lex_quoted_ident(l, start);
            } else if (is_alpha(peek(l, 0))) {
                lex_intrinsic(l, start);
            } else {
                lexer_error(l, start, "lonely `@'. expected a ident or a intrinsic.");
            }
            continue;
        }
        if (lex_punctuator(l, start)) {
            continue;
        }

        lexer_error(l, start, "Unexpected character '%c'.", c);
        consume(l, 1);
    }
    // add the last token.
    Location loc_end = location(l->ln, l->cn, l->i);
    StrView eot = sv(l->buffer, l->i);
    lexer_emit(l, loc_end, loc_end, END_OF_TOKEN, eot, (Value){0});
    // nulling the first token's last token ptr.
    l->token_head->next->last = NULL;

    if (l->errorc > 0) fatal("Lexer had %zu error(s).", l->errorc);

    return l->token_head->next;
}
