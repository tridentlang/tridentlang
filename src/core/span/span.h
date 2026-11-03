#pragma once

#include <stddef.h>

typedef struct {
    size_t ln;
    size_t cn;
    size_t i;
} Location;

typedef struct {
    Location start;
    Location end;
} Span;

static inline Location location(size_t ln, size_t cn, size_t i) {
    return (Location){ln, cn, i};
}

static inline Span span(Location start, Location end) { return (Span){start, end}; }

static inline Span span_merge(Span a, Span b) { return (Span){a.start, b.end}; }
