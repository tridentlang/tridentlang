#include "type.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_primitive_kind(const TokenKind kind) {
    switch (kind) {
    case KW_VOID:
    case KW_I8:
    case KW_I16:
    case KW_I32:
    case KW_I64:
    case KW_I128:
    case KW_ISIZE:
    case KW_U8:
    case KW_U16:
    case KW_U32:
    case KW_U64:
    case KW_U128:
    case KW_USIZE:
    case KW_BOOL:
    case KW_F16:
    case KW_F32:
    case KW_F64:
    case KW_F128:
    case KW_STR: return true;
    default: return false;
    }
}

PrimitiveKind primitive_kind(const TokenKind kind) {
    switch (kind) {
    case KW_VOID: return PRIMITIVE_VOID;
    case KW_I8: return PRIMITIVE_I8;
    case KW_I16: return PRIMITIVE_I16;
    case KW_I32: return PRIMITIVE_I32;
    case KW_I64: return PRIMITIVE_I64;
    case KW_I128: return PRIMITIVE_I128;
    case KW_ISIZE: return PRIMITIVE_ISIZE;
    case KW_U8: return PRIMITIVE_U8;
    case KW_U16: return PRIMITIVE_U16;
    case KW_U32: return PRIMITIVE_U32;
    case KW_U64: return PRIMITIVE_U64;
    case KW_U128: return PRIMITIVE_U128;
    case KW_USIZE: return PRIMITIVE_USIZE;
    case KW_BOOL: return PRIMITIVE_BOOL;
    case KW_F16: return PRIMITIVE_F16;
    case KW_F32: return PRIMITIVE_F32;
    case KW_F64: return PRIMITIVE_F64;
    case KW_F128: return PRIMITIVE_F128;
    case KW_STR: return PRIMITIVE_STR;
    default: return PRIMITIVE_NONE;
    }
}

Type *type_get(const TypePool *pool, TypeIndex index) {
    assert(index < pool->count);
    return &pool->types[index];
}

TypeIndex type_push(TypePool *pool, Type type) {
    if (pool->count == pool->capacity) {
        uint32_t capacity = pool->capacity ? pool->capacity * 2 : 64;
        Type *grown = realloc(pool->types, (size_t)capacity * sizeof *grown);
        if (!grown) {
            fprintf(stderr, "out of memory\n");
            abort();
        }
        pool->types = grown;
        pool->capacity = capacity;
    }
    pool->types[pool->count] = type;
    return pool->count++;
}

TypePool *init_type_pool(void) {
    TypePool *pool = xcalloc(1, sizeof *pool);
    pool->arena = init_arena((size_t)64 * 1024);

    for (int k = 0; k < PRIMITIVE_NONE; k++) {
        Type t = {0};
        t.kind = TYPE_PRIMITIVE;
        t.type = (TypeIndex)k;
        pool->primitives[k] = type_push(pool, t);
    }

    Type err = {0};
    err.kind = TYPE_ERROR;
    pool->error = type_push(pool, err);

    return pool;
}

void free_type_pool(TypePool *pool) {
    if (!pool) {
        return;
    }
    free(pool->types);
    free(pool->table);
    free_arena(pool->arena);
    free(pool);
}

TypeIndex type_primitive(TypePool *pool, PrimitiveKind kind) {
    if ((int)kind < 0 || kind >= PRIMITIVE_NONE) {
        return pool->error;
    }
    return pool->primitives[kind];
}

TypeIndex type_error(const TypePool *pool) { return pool->error; }

static uint64_t mix(uint64_t h, uint64_t v) {
    return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
}

static uint64_t hash_type(const Type *key) {
    uint64_t h = mix(0, (uint64_t)key->kind);
    h = mix(h, key->type);
    switch (key->kind) {
    case TYPE_ARRAY: h = mix(h, key->as.array_size); break;
    case TYPE_FUNCTION:
        h = mix(h, key->as.func.params_count);
        for (size_t i = 0; i < key->as.func.params_count; i++) {
            h = mix(h, key->as.func.params[i]);
        }
        break;
    default: break;
    }
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

static bool type_key_equal(const Type *a, const Type *b) {
    if (a->kind != b->kind || a->type != b->type) {
        return false;
    }
    switch (a->kind) {
    case TYPE_POINTER:
    case TYPE_SLICE: return true;
    case TYPE_ARRAY: return a->as.array_size == b->as.array_size;
    case TYPE_FUNCTION:
        if (a->as.func.params_count != b->as.func.params_count) {
            return false;
        }
        for (size_t i = 0; i < a->as.func.params_count; i++) {
            if (a->as.func.params[i] != b->as.func.params[i]) {
                return false;
            }
        }
        return true;
    default: return false;
    }
}

static void table_insert(TypePool *pool, TypeIndex index, uint64_t hash) {
    uint32_t mask = pool->table_capacity - 1;
    uint32_t i = (uint32_t)hash & mask;
    while (pool->table[i]) {
        i = (i + 1) & mask;
    }
    pool->table[i] = index + 1;
}

static void table_grow(TypePool *pool) {
    uint32_t old_capacity = pool->table_capacity;
    TypeIndex *old = pool->table;

    pool->table_capacity = old_capacity ? old_capacity * 2 : 64;
    pool->table = xcalloc(pool->table_capacity, sizeof *pool->table);

    for (uint32_t i = 0; i < old_capacity; i++) {
        if (old[i]) {
            TypeIndex index = old[i] - 1;
            table_insert(pool, index, hash_type(&pool->types[index]));
        }
    }
    free(old);
}

static TypeIndex type_intern(TypePool *pool, const Type *key) {
    if ((pool->table_used + 1) * 2 > pool->table_capacity) {
        table_grow(pool);
    }

    uint64_t hash = hash_type(key);
    uint32_t mask = pool->table_capacity - 1;
    uint32_t i = (uint32_t)hash & mask;

    while (pool->table[i]) {
        TypeIndex existing = pool->table[i] - 1;
        if (type_key_equal(&pool->types[existing], key)) {
            return existing;
        }
        i = (i + 1) & mask;
    }

    Type created = *key;
    if (key->kind == TYPE_FUNCTION && key->as.func.params_count > 0) {
        size_t bytes = key->as.func.params_count * sizeof(TypeIndex);
        TypeIndex *copy = arena_alloc(pool->arena, bytes);
        memcpy(copy, key->as.func.params, bytes);
        created.as.func.params = copy;
    }

    TypeIndex index = type_push(pool, created);
    pool->table[i] = index + 1;
    pool->table_used++;
    return index;
}

TypeIndex type_pointer(TypePool *pool, TypeIndex base) {
    Type key = {0};
    key.kind = TYPE_POINTER;
    key.type = base;
    return type_intern(pool, &key);
}

TypeIndex type_slice(TypePool *pool, TypeIndex base) {
    Type key = {0};
    key.kind = TYPE_SLICE;
    key.type = base;
    return type_intern(pool, &key);
}

TypeIndex type_array(TypePool *pool, TypeIndex base, size_t size) {
    Type key = {0};
    key.kind = TYPE_ARRAY;
    key.type = base;
    key.as.array_size = size;
    return type_intern(pool, &key);
}

TypeIndex type_function(TypePool *pool, const TypeIndex *params, size_t params_count,
                        TypeIndex ret_type) {
    Type key = {0};
    key.kind = TYPE_FUNCTION;
    key.type = ret_type;
    key.as.func.params = params;
    key.as.func.params_count = params_count;
    return type_intern(pool, &key);
}

static uint32_t name_hash(Name name) {
    uint64_t x = (uint64_t)(uintptr_t)name;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return (uint32_t)x;
}

static bool is_object_kind(TypeKind kind) {
    return kind == TYPE_STRUCTURE || kind == TYPE_UNION || kind == TYPE_ENUMERATION;
}

TypeIndex type_object_declare(TypePool *pool, TypeKind kind, Name name, bool anonymous) {
    assert(is_object_kind(kind));
    Type t = {0};
    t.kind = kind;
    t.as.object.name = name;
    t.as.object.anonymous = anonymous;
    t.as.object.state = OBJECT_DECLARED;
    return type_push(pool, t);
}

bool type_object_complete(TypePool *pool, TypeIndex object, const Member *members,
                          uint32_t members_count, uint32_t *dup_index) {
    Type *type = type_get(pool, object);
    assert(is_object_kind(type->kind));
    Object *o = &type->as.object;
    assert(o->state == OBJECT_DECLARED);

    if (members_count == 0) {
        o->members = NULL;
        o->members_count = 0;
        o->slots = NULL;
        o->slot_mask = 0;
        o->state = OBJECT_COMPLETE;
        return true;
    }

    uint32_t capacity = 2;
    while (capacity < members_count * 2) {
        capacity <<= 1;
    }

    Member *copy = arena_alloc(pool->arena, members_count * sizeof *copy);
    uint32_t *slots = arena_alloc(pool->arena, capacity * sizeof *slots);
    memset(slots, 0, capacity * sizeof *slots);

    bool ok = true;
    for (uint32_t i = 0; i < members_count; i++) {
        copy[i] = members[i];

        uint32_t h = name_hash(members[i].name) & (capacity - 1);
        bool duplicate = false;
        while (slots[h]) {
            if (copy[slots[h] - 1].name == members[i].name) {
                duplicate = true;
                break;
            }
            h = (h + 1) & (capacity - 1);
        }

        if (duplicate) {
            if (ok && dup_index) {
                *dup_index = i;
            }
            ok = false;
            continue;
        }
        slots[h] = i + 1;
    }

    o->members = copy;
    o->members_count = members_count;
    o->slots = slots;
    o->slot_mask = capacity - 1;
    o->state = OBJECT_COMPLETE;
    return ok;
}

const Member *type_find_member_index(const TypePool *pool, TypeIndex object, Name name,
                                     uint32_t *index) {
    const Type *t = type_get(pool, object);
    if (!is_object_kind(t->kind)) {
        return NULL;
    }
    const Object *o = &t->as.object;
    if (!o->slots) {
        return NULL;
    }

    uint32_t h = name_hash(name) & o->slot_mask;
    while (o->slots[h]) {
        const Member *m = &o->members[o->slots[h] - 1];
        if (m->name == name) {
            if (index) {
                *index = o->slots[h] - 1;
            }
            return m;
        }
        h = (h + 1) & o->slot_mask;
    }
    return NULL;
}

const Member *type_find_member(const TypePool *pool, TypeIndex object, Name name) {
    return type_find_member_index(pool, object, name, NULL);
}

Info *info(Arena *arena, InfoKind kind, TypeIndex type, bool is_mutable, Value val) {
    Info *info = arena_alloc(arena, sizeof(Info));
    *info = (Info){kind, type, is_mutable, val};
    return info;
}
