#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "core/sv/sv.h"
#include "process.h"

static size_t parse_int(StrView sv, bool *overflow) {
    char buf[128];
    *overflow = false;

    if (sv.len >= sizeof buf) {
        *overflow = true;
        return 0;
    }

    memcpy(buf, sv.str, sv.len);
    buf[sv.len] = '\0';

    errno = 0;
    unsigned long long v;
    if (buf[0] == '0' && (buf[1] == 'b' || buf[1] == 'B')) {
        v = strtoull(buf + 2, NULL, 2);
    } else if (buf[0] == '0' && (buf[1] == 'o' || buf[1] == 'O')) {
        v = strtoull(buf + 2, NULL, 8);
    } else {
        v = strtoull(buf, NULL, 0);
    }

    if (errno == ERANGE || v > (unsigned long long)SIZE_MAX) {
        *overflow = true;
        return 0;
    }
    return (size_t)v;
}

static double parse_float(StrView sv) {
    char stack[128];
    char *buf = stack;

    if (sv.len >= sizeof stack) {
        buf = malloc(sv.len + 1);
        if (!buf) return 0.0;
    }

    memcpy(buf, sv.str, sv.len);
    buf[sv.len] = '\0';

    const double v = strtod(buf, NULL);

    if (buf != stack) free(buf);
    return v;
}

static void lex_radix_digits(Lexer *l, Location start, bool (*is_valid)(char),
                             const char *empty_msg) {
    consume(l, 2);
    if (consume_while(l, is_valid) == 0) lexer_error(l, start, "%s", empty_msg);
}

static bool lex_fraction_exponent(Lexer *l) {
    bool is_float = false;

    if (peek(l, 0) == '.' && peek(l, 1) != '.') {
        is_float = true;
        consume(l, 1);
        consume_while(l, is_digit);
    }

    const char e = peek(l, 0);
    if (e == 'e' || e == 'E') {
        const char sign = peek(l, 1);
        const int off = (sign == '+' || sign == '-') ? 2 : 1;
        if (is_digit(peek(l, off))) {
            is_float = true;
            consume(l, off);
            consume_while(l, is_digit);
        }
    }

    return is_float;
}

void lex_number(Lexer *l, Location start) {
    const char c0 = peek(l, 0);
    const char next = peek(l, 1);
    bool is_float = false;
    bool is_radix = false;

    if (c0 == '0' && (next == 'x' || next == 'X')) {
        is_radix = true;
        lex_radix_digits(l, start, is_xdigit, "Hex constant has no digits.");
    } else if (c0 == '0' && (next == 'b' || next == 'B')) {
        is_radix = true;
        lex_radix_digits(l, start, is_bin_digit, "Binary constant has no digits.");
    } else if (c0 == '0' && (next == 'o' || next == 'O')) {
        is_radix = true;
        lex_radix_digits(l, start, is_octal, "Octal constant has no digits.");
    } else {
        consume_while(l, is_digit);
        is_float = lex_fraction_exponent(l);
        if (is_float) {
            const char s = peek(l, 0);
            if (s == 'f' || s == 'F' || s == 'l' || s == 'L') consume(l, 1);
        }
    }

    if (is_ident_char(peek(l, 0))) {
        lexer_error(l, start, "Invalid character `%c` in numeric constant.", peek(l, 0));
        consume_while(l, is_ident_char);
    }

    StrView lexeme = sv_at(l->buffer, start.i, l->i - start.i);
    const Location end = location(l->ln, l->cn, l->i);

    if (is_float) {
        Value val;
        val.f = parse_float(lexeme);
        lexer_emit(l, start, end, FLOAT_LIT, lexeme, val);
        return;
    }

    if (!is_radix && lexeme.len > 1 && lexeme.str[0] == '0') {
        for (size_t k = 1; k < lexeme.len; k++) {
            if (!is_octal(lexeme.str[k])) {
                lexer_error(l, start, "Invalid digit '%c' in octal constant.",
                            lexeme.str[k]);
                break;
            }
        }
    }

    bool overflow;
    Value val;
    val.i = parse_int(lexeme, &overflow);
    if (overflow) lexer_error(l, start, "Integer constant is too large.");
    lexer_emit(l, start, end, INT_LIT, lexeme, val);
}
