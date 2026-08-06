#ifndef TEST_H
#define TEST_H

#include "types.h"

extern volatile uint32_t test_ticks;

typedef void (*test_func_t)(void);

typedef struct
{
    const char *name;
    test_func_t func;

} test_entry_t;

void test_begin(const char *suite);
void test_end(void);

void test_case(const char *name);

void test_pass(void);
void test_fail(void);

void test_assertion(const char *expr);

void test_summary(void);

void test_wait_ticks(uint32_t ticks);

#define TEST_ASSERT(expr)                 \
do                                    \
{                                     \
    test_assertion(#expr);            \
    \
    if (!(expr))                      \
    {                                 \
        test_fail();                  \
        return;                       \
    }                                 \
} while (0)

#define TEST_ASSERT_NULL(ptr)         \
TEST_ASSERT((ptr) == NULL)

#define TEST_ASSERT_NOT_NULL(ptr)     \
TEST_ASSERT((ptr) != NULL)

#define TEST_ASSERT_EQ(a, b)          \
TEST_ASSERT((a) == (b))

#define TEST_ASSERT_NE(a, b)          \
TEST_ASSERT((a) != (b))

#define TEST_ASSERT_TRUE(expr)        \
TEST_ASSERT((expr))

#define TEST_ASSERT_FALSE(expr)       \
TEST_ASSERT(!(expr))

#endif
