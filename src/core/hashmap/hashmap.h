#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct HashMap HashMap;

uint64_t hash_bytes(const void *data, size_t len);

HashMap *init_hash_map(size_t capacity);

int put_to_hash_map(HashMap *map, const void *key, size_t key_len, void *value);

void *get_from_hash_map(const HashMap *map, const void *key, size_t key_len);

int has_in_hash_map(const HashMap *map, const void *key, size_t key_len);

int del_from_hash_map(HashMap *map, const void *key, size_t key_len);

int resize_hash_map(HashMap *map, size_t capacity);

size_t hash_map_count(const HashMap *map);

void free_hash_map(HashMap *map);

void free_hash_map_all(HashMap *map, void (*value_free)(void *));
