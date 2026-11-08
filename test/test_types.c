#include "syntax/token/token.h"
#include "test.h"
#include "type/type.h"

/* Distinct arrays => distinct pointers, standing in for interned identifiers. */
static const char N_X[] = "x", N_Y[] = "y", N_NEXT[] = "next", N_Z[] = "z";

TEST(primitives_are_singletons) {
    TypePool *p = init_type_pool();
    CHECK_EQ(type_primitive(p, PRIMITIVE_I32), type_primitive(p, PRIMITIVE_I32));
    CHECK(type_primitive(p, PRIMITIVE_I32) != type_primitive(p, PRIMITIVE_I64));
    CHECK_EQ(type_get(p, type_primitive(p, PRIMITIVE_BOOL))->kind, TYPE_PRIMITIVE);
    CHECK_EQ(type_get(p, type_primitive(p, PRIMITIVE_BOOL))->type,
             (TypeIndex)PRIMITIVE_BOOL);
    CHECK_EQ(type_primitive(p, PRIMITIVE_NONE), type_error(p));
    CHECK_EQ(type_get(p, type_error(p))->kind, TYPE_ERROR);
    free_type_pool(p);
}

TEST(pointers_slices_arrays_intern) {
    TypePool *p = init_type_pool();
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    TypeIndex u8 = type_primitive(p, PRIMITIVE_U8);

    CHECK_EQ(type_pointer(p, i32), type_pointer(p, i32));
    CHECK(type_pointer(p, i32) != type_pointer(p, u8));
    CHECK(type_pointer(p, i32) != type_slice(p, i32)); /* same base, different kind */
    CHECK_EQ(type_slice(p, i32), type_slice(p, i32));
    CHECK_EQ(type_array(p, i32, 4), type_array(p, i32, 4));
    CHECK(type_array(p, i32, 4) != type_array(p, i32, 5));
    CHECK(type_array(p, i32, 4) != type_array(p, u8, 4));
    CHECK_EQ(type_pointer(p, type_pointer(p, i32)),
             type_pointer(p, type_pointer(p, i32)));
    CHECK_EQ(type_get(p, type_array(p, i32, 4))->as.array_size, (size_t)4);
    free_type_pool(p);
}

TEST(function_types) {
    TypePool *p = init_type_pool();
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    TypeIndex i64 = type_primitive(p, PRIMITIVE_I64);
    TypeIndex u8 = type_primitive(p, PRIMITIVE_U8);
    TypeIndex b = type_primitive(p, PRIMITIVE_BOOL);

    TypeIndex ps[2] = {i32, i64};
    TypeIndex f1 = type_function(p, ps, 2, b);
    ps[0] = u8; /* mutate the caller's array: the interned copy must not change */
    CHECK_EQ(type_get(p, f1)->as.func.params[0], i32);

    TypeIndex ps2[2] = {i32, i64};
    CHECK_EQ(type_function(p, ps2, 2, b), f1);
    CHECK(type_function(p, ps2, 2, i32) != f1); /* different return type */
    CHECK(type_function(p, ps2, 1, b) != f1);   /* different param count */
    TypeIndex ps3[2] = {i64, i32};
    CHECK(type_function(p, ps3, 2, b) != f1); /* order matters */
    CHECK_EQ(type_function(p, NULL, 0, b), type_function(p, NULL, 0, b));
    free_type_pool(p);
}

TEST(growth_keeps_identity) {
    enum { N = 5000 };
    static TypeIndex first[N];
    TypePool *p = init_type_pool();
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    for (int n = 0; n < N; n++)
        first[n] = type_array(p, i32, (size_t)n);
    uint32_t count = p->count;
    int same = 0;
    for (int n = 0; n < N; n++)
        same += type_array(p, i32, (size_t)n) == first[n];
    CHECK_EQ(same, N);
    CHECK_EQ(p->count, count); /* re-interning created nothing */
    CHECK(first[0] != first[1]);
    free_type_pool(p);
}

TEST(struct_member_lookup) {
    TypePool *p = init_type_pool();
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    TypeIndex s = type_object_declare(p, TYPE_STRUCTURE, "Point", false);
    Member m[] = {{N_X, i32}, {N_Y, type_primitive(p, PRIMITIVE_F32)}};
    uint32_t dup = 0, idx = 99;
    CHECK(type_object_complete(p, s, m, 2, &dup));

    const Member *fx = type_find_member_index(p, s, N_X, &idx);
    CHECK(fx != NULL);
    CHECK_EQ(idx, 0u);
    CHECK_EQ(fx->type, i32);
    CHECK(type_find_member_index(p, s, N_Y, &idx) != NULL);
    CHECK_EQ(idx, 1u);
    CHECK(type_find_member(p, s, N_Z) == NULL);
    CHECK(type_find_member(p, i32, N_X) == NULL); /* not an object type */
    free_type_pool(p);
}

TEST(recursive_struct) {
    TypePool *p = init_type_pool();
    TypeIndex node = type_object_declare(p, TYPE_STRUCTURE, "Node", false);
    TypeIndex next = type_pointer(p, node); /* node is only declared so far */
    Member m[] = {{N_X, type_primitive(p, PRIMITIVE_I32)}, {N_NEXT, next}};
    uint32_t dup;
    CHECK(type_object_complete(p, node, m, 2, &dup));
    const Member *n = type_find_member(p, node, N_NEXT);
    CHECK(n != NULL);
    CHECK_EQ(n->type, next);
    CHECK_EQ(type_get(p, n->type)->type, node);
    free_type_pool(p);
}

TEST(nominal_types_are_distinct) {
    TypePool *p = init_type_pool();
    TypeIndex a = type_object_declare(p, TYPE_STRUCTURE, "A", false);
    TypeIndex b = type_object_declare(p, TYPE_STRUCTURE, "A", false);
    CHECK(a != b); /* same name and fields would still be different declarations */
    free_type_pool(p);
}

TEST(duplicate_member_detected) {
    TypePool *p = init_type_pool();
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    TypeIndex i64 = type_primitive(p, PRIMITIVE_I64);
    TypeIndex s = type_object_declare(p, TYPE_STRUCTURE, "Bad", false);
    Member m[] = {{N_X, i32}, {N_Y, i32}, {N_X, i64}};
    uint32_t dup = 99;
    CHECK(!type_object_complete(p, s, m, 3, &dup));
    CHECK_EQ(dup, 2u);
    const Member *fx = type_find_member(p, s, N_X);
    CHECK(fx != NULL);
    CHECK_EQ(fx->type, i32); /* first definition wins */
    CHECK_EQ(type_get(p, s)->as.object.state, OBJECT_COMPLETE);
    free_type_pool(p);
}

TEST(empty_and_big_objects) {
    TypePool *p = init_type_pool();
    TypeIndex e = type_object_declare(p, TYPE_STRUCTURE, "Empty", false);
    uint32_t dup;
    CHECK(type_object_complete(p, e, NULL, 0, &dup));
    CHECK(type_find_member(p, e, N_X) == NULL);

    enum { N = 300 };
    static char names[N][8];
    static Member m[N];
    TypeIndex i32 = type_primitive(p, PRIMITIVE_I32);
    for (int i = 0; i < N; i++) {
        snprintf(names[i], sizeof names[i], "f%d", i);
        m[i] = (Member){names[i], i32};
    }
    TypeIndex big = type_object_declare(p, TYPE_UNION, "Big", false);
    CHECK(type_object_complete(p, big, m, N, &dup));
    int found = 0;
    for (int i = 0; i < N; i++) {
        uint32_t idx;
        const Member *f = type_find_member_index(p, big, names[i], &idx);
        found += f && idx == (uint32_t)i;
    }
    CHECK_EQ(found, N);
    CHECK(type_find_member(p, big, N_Z) == NULL);
    free_type_pool(p);
}

TEST(info_alloc) {
    TypePool *p = init_type_pool();
    Info *i =
        info(p->arena, PARAM_VAR, type_primitive(p, PRIMITIVE_I8), false, (Value){0});
    CHECK_EQ(i->kind, PARAM_VAR);
    CHECK_EQ(i->type, type_primitive(p, PRIMITIVE_I8));
    free_type_pool(p);
}

int main(void) {
    RUN(primitives_are_singletons);
    RUN(pointers_slices_arrays_intern);
    RUN(function_types);
    RUN(growth_keeps_identity);
    RUN(struct_member_lookup);
    RUN(recursive_struct);
    RUN(nominal_types_are_distinct);
    RUN(duplicate_member_detected);
    RUN(empty_and_big_objects);
    RUN(info_alloc);
    return TEST_MAIN_END();
}
