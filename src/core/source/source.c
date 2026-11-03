#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/def.h"
#include "core/diags/diags.h"
#include "core/mem/mem.h"
#include "core/sv/sv.h"
#include "source.h"

FILE *openf(const char *path, uint8_t mode) {
    const char *mode_str = mode ? "rb" : "wa";
    FILE *f = fopen(path, mode_str);
    if (!f) fatal("File not found! " FG_GREEN "\"%s\"" RESET, path);
    return f;
}

char *readf(FILE *file, size_t *bytes) {
    fseek(file, 0, SEEK_END);
    const size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!content) fatal("Failed to allocate %zu bytes.", file_size + 1);
    const size_t bytes_red = fread(content, 1, file_size, file);
    if (bytes_red != file_size) fatal("Failed to read file.");
    content[file_size] = '\0';

    if (bytes) {
        *bytes = bytes_red;
    }

    return content;
}

Source *init_source(Source *s, const char *file_path) {
    FILE *file = openf(file_path, 1);
    size_t buf_len = 0;
    const char *content = readf(file, &buf_len);

    if (!content) return NULL;

    s->file_path = file_path;
    s->buffer = content;
    s->buffer_len = buf_len;

    if (buf_len > 0) {
        s->nlines = 1;
        for (size_t i = 0; i < buf_len; ++i) {
            if (content[i] == '\n') ++s->nlines;
        }
    }

    s->lines = xmalloc(sizeof *s->lines * (s->nlines ? s->nlines : 1));

    size_t line_idx = 0, start = 0;
    for (size_t i = 0; i < s->buffer_len; ++i) {
        if (s->buffer[i] == '\n') {
            s->lines[line_idx++] = sv(s->buffer + start, i - start);
            start = i + 1;
        }
    }

    if (s->nlines > 0)
        s->lines[line_idx++] = sv(s->buffer + start, s->buffer_len - start);

    return s;
}

void free_source(Source *s) {
    if (!s) return;
    free((char *)s->buffer);
    free(s->lines);
}
