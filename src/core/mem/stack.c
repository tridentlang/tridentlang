#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "mem.h"

struct Stack {
    void **data;
    size_t size;
    size_t cap;
};

Stack *init_stack(Stack *stack, size_t stack_size) {
    if (stack_size == 0) stack_size = 8;
    stack->data = (void **)malloc(stack_size * sizeof(void *));
    stack->cap = stack->data ? stack_size : 0;
    stack->size = 0;
    return stack->data ? stack : NULL;
}

bool stack_push(Stack *stack, void *element) {
    if (stack->size >= stack->cap) {
        size_t new_cap = stack->cap ? stack->cap * 2 : 8;
        void **new_data = (void **)realloc((void *)stack->data, new_cap * sizeof(void *));
        if (!new_data) return false;
        stack->data = new_data;
        stack->cap = new_cap;
    }
    stack->data[stack->size++] = element;
    return true;
}

void *stack_pop(Stack *stack) {
    if (stack->size == 0) return NULL;
    return stack->data[--stack->size];
}

size_t stack_size(const Stack *stack) { return stack->size; }

void free_stack(Stack *stack) {
    free((void *)stack->data);
    stack->data = NULL;
    stack->size = stack->cap = 0;
}
