#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "core/sv/sv.h"

typedef struct {
    const char *file_path;
    const char *buffer;
    size_t buffer_len;
    StrView *lines;
    size_t nlines;
} Source;

Source *init_source(Source *s, const char *file_path);
void free_source(Source *src);

FILE *openf(const char *path, uint8_t mode);
char *readf(FILE *file, size_t *bytes);
