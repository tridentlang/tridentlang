#include <stdio.h>

#include "token.h"

#include "core/def.h"
#include "core/sv/sv.h"

Token token(TokenKind kind, StrView lexeme, Span span, Value val) {
    return (Token){kind, lexeme, span, val, NULL, NULL};
}

const char *token_kind_to_str(TokenKind kind) {
    static const char *const names[] = {
#define LITERAL(tok, name) [tok##_LIT] = (name),
#define KEYWORD(tok, name) [KW_##tok] = (name),
#define PUNCTUATION(tok, name) [PUNCT_##tok] = (name),
#define TOKEN(tok, name) [tok] = (name),
#define INTRINSIC(tok, name) [IC_##tok] = (name),
#include "token.def"
#undef INTRINSIC
#undef TOKEN
#undef LITERAL
#undef KEYWORD
#undef PUNCTUATION
    };

    size_t count = sizeof(names) / sizeof(names[0]);
    if ((size_t)kind < count && names[kind] != NULL) return names[kind];
    return "UNKNOWN_TOKEN";
}

void print_token(const Token *tok) {
    fprintf(stderr, "%s\t", token_kind_to_str(tok->kind));
    const StrView text = tok->lexeme;

    switch (tok->kind) {
#define LITERAL(t, n)
#define TOKEN(t, n)
#define KEYWORD(t, n)
#define INTRINSIC(t, n)                                                                  \
    case IC_##t: fprintf(stderr, COL_INTRINSIC "@%.*s", (int)text.len, text.str); break;
#define PUNCTUATION(t, n)                                                                \
    case PUNCT_##t: fprintf(stderr, COL_PUNCT "%s", n); break;
#include "token.def"
#undef INTRINSIC
#undef TOKEN
#undef LITERAL
#undef KEYWORD
#undef PUNCTUATION

    case KW_VOID:
    case KW_I8:
    case KW_I16:
    case KW_I32:
    case KW_I64:
    case KW_I128:
    case KW_ISIZE:
    case KW_U8:
    case KW_U16:
    case KW_U32:
    case KW_U64:
    case KW_U128:
    case KW_USIZE:
    case KW_BOOL:
    case KW_F16:
    case KW_F32:
    case KW_F64:
    case KW_F128:
    case KW_STR: fprintf(stderr, COL_TYPE SV_FMT, SV_ARG(text)); break;

    case KW_IF:
    case KW_ELSE:
    case KW_LOOP:
    case KW_WHILE:
    case KW_MATCH:
    case KW_CASE:
    case KW_BREAK:
    case KW_CONTINUE:
    case KW_RETURN: fprintf(stderr, COL_CONTROL SV_FMT, SV_ARG(text)); break;

    case KW_ENUM:
    case KW_UNION:
    case KW_STRUCT:
    case KW_ALIAS:
    case KW_PUB:
    case KW_VOLATILE:
    case KW_CONST:
    case KW_MUT:
    case KW_FUNC: fprintf(stderr, COL_DECL SV_FMT, SV_ARG(text)); break;

    case KW_TRUE:
    case KW_FALSE: fprintf(stderr, COL_BOOL SV_FMT, SV_ARG(text)); break;

    case IDENTIFIER_LIT: fprintf(stderr, COL_IDENT SV_FMT, SV_ARG(text)); break;
    case CHARACTER_LIT: fprintf(stderr, COL_CHAR "'%c'", (char)tok->val.i); break;
    case INT_LIT: fprintf(stderr, COL_NUMBER "%zu", (size_t)tok->val.i); break;
    case FLOAT_LIT: fprintf(stderr, COL_NUMBER "%g", tok->val.f); break;
    case STRING_LIT: fprintf(stderr, COL_STRING "\"" SV_FMT "\"", SV_ARG(text)); break;

    case END_OF_TOKEN: break;

    default: fprintf(stderr, COL_UNKNOWN SV_FMT, SV_ARG(text)); break;
    }
    fprintf(stderr, RESET);
}

void print_tokens(const Token *tokens) {
    for (const Token *cur = tokens; cur != NULL; cur = cur->next) {
        fprintf(stderr, "%zu:%zu:%zu:%zu\t", cur->span.start.ln, cur->span.start.cn,
                cur->span.start.i, cur->span.start.i - cur->span.end.i);
        print_token(cur);
        fprintf(stderr, "\n");
    }
}
