#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *old, size_t n);
void xfree(void *ptr);

char *xstrndup(const char *s, size_t n);

#ifndef ARENA_REGION_DEFAULT_CAPACITY
#define ARENA_REGION_DEFAULT_CAPACITY (1024)
#endif

typedef struct Arena Arena;

Arena *init_arena(size_t region_size);
void *arena_alloc(Arena *arena, size_t size);
void free_arena(Arena *arena);

typedef struct Stack Stack;

Stack *init_stack(Stack *stack, size_t stack_size);
bool stack_push(Stack *stack, void *element);
void *stack_pop(Stack *stack);
size_t stack_size(const Stack *stack);
void free_stack(Stack *stack);

char *xstrdup(const char *s);
char *substr(Arena *a, const char *source, size_t start, size_t length);
