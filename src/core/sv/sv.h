#pragma once

#include <stddef.h>
typedef struct StrView StrView;

struct StrView {
    const char *str;
    size_t len;
};

static inline StrView sv(const char *str, size_t len) {
    return (StrView){.str = str, .len = len};
}

static inline StrView sv_at(const char *base, size_t offset, size_t len) {
    return (StrView){.str = base + offset, .len = len};
}

static inline int sv_len(StrView *self) { return (int)self->len; }

static inline const char *sv_str(StrView *self) { return self->str; }

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).len, (sv).str
