#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define HM_MIN_CAP 8

typedef struct {
    const void *key;
    void *value;
    uint64_t hash;
    size_t key_len;
} Entry;

struct HashMap {
    Entry *entries;
    size_t cap;
    size_t count;
};

uint64_t hash_bytes(const void *data, size_t len) {
    const unsigned char *p = data;
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    h ^= h >> 32;
    return h;
}

static size_t round_pow2(size_t n) {
    size_t cap = HM_MIN_CAP;
    while (cap < n) {
        if (cap > SIZE_MAX / 2) return 0;
        cap <<= 1;
    }
    return cap;
}

static Entry *find(const HashMap *map, const void *key, size_t key_len, uint64_t hash) {
    size_t mask = map->cap - 1;
    size_t i = (size_t)hash & mask;
    for (;;) {
        Entry *e = &map->entries[i];
        if (!e->key) return NULL;
        if (e->hash == hash && e->key_len == key_len &&
            (e->key == key || memcmp(e->key, key, key_len) == 0))
            return e;
        i = (i + 1) & mask;
    }
}

HashMap *init_hash_map(size_t capacity) {
    size_t cap = round_pow2(capacity);
    if (!cap) return NULL;

    HashMap *map = malloc(sizeof *map);
    if (!map) return NULL;

    map->entries = calloc(cap, sizeof *map->entries);
    if (!map->entries) {
        free(map);
        return NULL;
    }

    map->cap = cap;
    map->count = 0;
    return map;
}

int resize_hash_map(HashMap *map, size_t capacity) {
    if (!map) return 0;

    size_t min_cap = map->count + map->count / 3 + 1;
    if (capacity < min_cap) capacity = min_cap;

    size_t cap = round_pow2(capacity);
    if (!cap) return 0;
    if (cap == map->cap) return 1;

    Entry *entries = calloc(cap, sizeof *entries);
    if (!entries) return 0;

    size_t mask = cap - 1;
    for (size_t i = 0; i < map->cap; ++i) {
        Entry *src = &map->entries[i];
        if (!src->key) continue;
        size_t j = (size_t)src->hash & mask;
        while (entries[j].key)
            j = (j + 1) & mask;
        entries[j] = *src;
    }

    free(map->entries);
    map->entries = entries;
    map->cap = cap;
    return 1;
}

int put_to_hash_map(HashMap *map, const void *key, size_t key_len, void *value) {
    if (!map || !key) return 0;

    uint64_t hash = hash_bytes(key, key_len);

    Entry *existing = find(map, key, key_len, hash);
    if (existing) {
        existing->value = value;
        return 1;
    }

    if ((map->count + 1) * 4 > map->cap * 3) {
        if (!resize_hash_map(map, map->cap * 2)) return 0;
    }

    size_t mask = map->cap - 1;
    size_t i = (size_t)hash & mask;
    while (map->entries[i].key)
        i = (i + 1) & mask;

    map->entries[i] = (Entry){key, value, hash, key_len};
    ++map->count;
    return 1;
}

void *get_from_hash_map(const HashMap *map, const void *key, size_t key_len) {
    if (!map || !key) return NULL;
    Entry *e = find(map, key, key_len, hash_bytes(key, key_len));
    return e ? e->value : NULL;
}

int has_in_hash_map(const HashMap *map, const void *key, size_t key_len) {
    if (!map || !key) return 0;
    return find(map, key, key_len, hash_bytes(key, key_len)) != NULL;
}

int del_from_hash_map(HashMap *map, const void *key, size_t key_len) {
    if (!map || !key) return 0;

    Entry *e = find(map, key, key_len, hash_bytes(key, key_len));
    if (!e) return 0;

    size_t mask = map->cap - 1;
    size_t i = (size_t)(e - map->entries);
    size_t j = i;

    for (;;) {
        j = (j + 1) & mask;
        if (!map->entries[j].key) break;
        size_t k = (size_t)map->entries[j].hash & mask;
        int stays = i <= j ? (i < k && k <= j) : (i < k || k <= j);
        if (stays) continue;
        map->entries[i] = map->entries[j];
        i = j;
    }

    map->entries[i].key = NULL;
    --map->count;
    return 1;
}

size_t hash_map_count(const HashMap *map) { return map ? map->count : 0; }

void free_hash_map(HashMap *map) {
    if (!map) return;
    free(map->entries);
    free(map);
}

void free_hash_map_all(HashMap *map, void (*value_free)(void *)) {
    if (!map) return;
    if (value_free) {
        for (size_t i = 0; i < map->cap; ++i)
            if (map->entries[i].key) value_free(map->entries[i].value);
    }
    free(map->entries);
    free(map);
}
