#pragma once

#include <stdio.h>
#include <stdlib.h>

static int t_fail, t_run;

#define CHECK(c)                                                                         \
    do {                                                                                 \
        if (!(c)) {                                                                      \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);               \
            ++t_fail;                                                                    \
        }                                                                                \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))

#define CHECK_STR(a, b) CHECK(strcmp((a), (b)) == 0)

#define TEST(name) static void name(void)

#define RUN(name)                                                                        \
    do {                                                                                 \
        ++t_run;                                                                         \
        fprintf(stderr, "%s\n", #name);                                                  \
        name();                                                                          \
    } while (0)

#define TEST_MAIN_END()                                                                  \
    (fprintf(stderr, "%d tests, %d failures\n", t_run, t_fail), t_fail ? 1 : 0)
