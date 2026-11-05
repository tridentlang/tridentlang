#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "core/diags/diags.h"

#define ARENA_NULL ((size_t)SIZE_MAX)

struct Arena {
    size_t initial_cap;
    size_t cap;
    size_t offset;
    uintptr_t *data;
};

static int bytes_to_words(size_t bytes, size_t *out_words) {
    if (bytes > SIZE_MAX - (sizeof(uintptr_t) - 1)) {
        return 0;
    }
    *out_words = (bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
    return 1;
}

static void arena_reserve(Arena *arena, size_t needed) {
    if (needed <= arena->cap) {
        return;
    }

    size_t new_cap = arena->cap ? arena->cap : arena->initial_cap;
    while (new_cap < needed) {
        if (new_cap > SIZE_MAX / sizeof(uintptr_t) / 2) {
            new_cap = needed;
            break;
        }
        new_cap *= 2;
    }

    if (new_cap > SIZE_MAX / sizeof(uintptr_t)) {
        fatal("Arena too large");
    }

    size_t bytes = new_cap * sizeof(uintptr_t);
    arena->data = arena->data ? xrealloc(arena->data, bytes) : xmalloc(bytes);
    arena->cap = new_cap;
}

Arena *init_arena(size_t initial_size_bytes) {
    size_t words = ARENA_REGION_DEFAULT_CAPACITY;
    if (initial_size_bytes > 0 && !bytes_to_words(initial_size_bytes, &words)) {
        return NULL;
    }

    Arena *arena = xmalloc(sizeof(Arena));
    *arena = (Arena){
        .initial_cap = words,
        .cap = 0,
        .offset = 0,
        .data = NULL,
    };
    return arena;
}

size_t arena_alloc_offset(Arena *arena, size_t size_bytes) {
    if (!arena || size_bytes == 0) {
        return ARENA_NULL;
    }

    size_t size_words;
    if (!bytes_to_words(size_bytes, &size_words) ||
        size_words > SIZE_MAX - arena->offset) {
        return ARENA_NULL;
    }

    arena_reserve(arena, arena->offset + size_words);

    size_t off = arena->offset;
    arena->offset += size_words;
    return off * sizeof(uintptr_t);
}

void *arena_ptr(Arena *arena, size_t byte_offset) {
    if (!arena || byte_offset == ARENA_NULL) {
        return NULL;
    }
    return (char *)arena->data + byte_offset;
}

void *arena_alloc(Arena *arena, size_t size_bytes) {
    size_t off = arena_alloc_offset(arena, size_bytes);
    return arena_ptr(arena, off);
}

void arena_reset(Arena *arena) {
    if (arena) {
        arena->offset = 0;
    }
}

void free_arena(Arena *arena) {
    if (!arena) {
        return;
    }
    if (arena->data) {
        xfree(arena->data);
    }
    xfree(arena);
}
