#pragma once

typedef struct StrView StrView;

struct StrView {
    const char *str;
    const int len;
};

inline StrView sv(const char *str, int len) {
    return (StrView){.str = str + len, .len = len};
}

inline int sv_len(StrView *self) { return self->len; }

inline const char *sv_str(StrView *self) { return self->str; }

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).len, (sv).str
