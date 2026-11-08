#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/mem/mem.h"
#include "syntax/token/token.h"

typedef enum {
    TYPE_PRIMITIVE,
    TYPE_POINTER,
    TYPE_SLICE,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCTURE,
    TYPE_UNION,
    TYPE_ENUMERATION,
    TYPE_ERROR,
} TypeKind;

typedef enum {
    PRIMITIVE_VOID,
    PRIMITIVE_I8,
    PRIMITIVE_I16,
    PRIMITIVE_I32,
    PRIMITIVE_I64,
    PRIMITIVE_I128,
    PRIMITIVE_ISIZE,
    PRIMITIVE_U8,
    PRIMITIVE_U16,
    PRIMITIVE_U32,
    PRIMITIVE_U64,
    PRIMITIVE_U128,
    PRIMITIVE_USIZE,
    PRIMITIVE_BOOL,
    PRIMITIVE_F16,
    PRIMITIVE_F32,
    PRIMITIVE_F64,
    PRIMITIVE_F128,
    PRIMITIVE_STR,
    PRIMITIVE_NONE, // for counting primitive types
} PrimitiveKind;

typedef uint32_t TypeIndex;

typedef const char *Name;

typedef struct {
    Name name;
    TypeIndex type;
} Member;

typedef enum {
    OBJECT_DECLARED,
    OBJECT_COMPLETE,
} ObjectState;

typedef struct {
    Name name;
    bool anonymous;
    ObjectState state;

    Member *members;
    uint32_t members_count;

    uint32_t *slots;
    uint32_t slot_mask;
} Object;

typedef struct Type Type;

struct Type {
    TypeKind kind;

    /* TYPE_PRIMITIVE: the PrimitiveKind
     * TYPE_POINTER, TYPE_SLICE, TYPE_ARRAY: element type
     * TYPE_FUNCTION: return type */
    TypeIndex type;

    union {
        /* TYPE_ARRAY */
        size_t array_size;

        /* TYPE_FUNCTION */
        struct {
            const TypeIndex *params;
            size_t params_count;
        } func;

        /* TYPE_STRUCTURE, TYPE_UNION, TYPE_ENUMERATION */
        Object object;
    } as;
};

typedef struct {
    Arena *arena;

    Type *types;
    uint32_t count;
    uint32_t capacity;

    TypeIndex primitives[PRIMITIVE_NONE];
    TypeIndex error;

    TypeIndex *table;
    uint32_t table_capacity;
    uint32_t table_used;
} TypePool;

typedef enum {
    STATIC_VAR,
    PARAM_VAR,
    FRAME_VAR,
} InfoKind;

typedef struct {
    InfoKind kind;
    TypeIndex type;
    bool is_mutable;
    Value val; // for constants.
} Info;

bool is_primitive_kind(const TokenKind kind);
TypeIndex primitive_kind(const TokenKind kind);

TypePool *init_type_pool(void);
void free_type_pool(TypePool *pool);

Type *type_get(const TypePool *pool, TypeIndex index);
TypeIndex type_push(TypePool *pool, Type type);

TypeIndex type_primitive(TypePool *pool, PrimitiveKind kind);
TypeIndex type_error(const TypePool *pool);
TypeIndex type_pointer(TypePool *pool, TypeIndex base);
TypeIndex type_slice(TypePool *pool, TypeIndex base);
TypeIndex type_array(TypePool *pool, TypeIndex base, size_t size);
TypeIndex type_function(TypePool *pool, const TypeIndex *params, size_t params_count,
                        TypeIndex ret_type);

TypeIndex type_object_declare(TypePool *pool, TypeKind kind, Name name, bool anonymous);

bool type_object_complete(TypePool *pool, TypeIndex object, const Member *members,
                          uint32_t members_count, uint32_t *dup_index);

const Member *type_find_member(const TypePool *pool, TypeIndex object, Name name);
const Member *type_find_member_index(const TypePool *pool, TypeIndex object, Name name,
                                     uint32_t *index);

Info *info(Arena *arena, InfoKind kind, TypeIndex type, bool is_mutable, Value val);
