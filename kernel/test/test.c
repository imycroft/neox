#include "test.h"
#include "printf.h"

volatile uint32_t test_ticks = 0;

static uint32_t tests_passed;
static uint32_t tests_failed;

static const char *current_test;
static const char *current_assertion;

void test_begin(const char *suite)
{
    printf("\n===== %s =====\n", suite);
}

void test_end(void)
{
    printf("====================\n\n");
}

void test_pass(void)
{
    tests_passed++;

    printf("[PASS] %s\n", current_test);
}

void test_fail(void)
{
    tests_failed++;

    printf("[FAIL] %s\n", current_test);

    printf("       Assertion: %s\n",
           current_assertion);
}

void test_case(const char *name)
{
    current_test = name;
}

void test_assertion(const char *expr)
{
    current_assertion = expr;
}

void test_summary(void)
{
    printf("\n========== TEST SUMMARY ==========\n");

    printf("Passed : %u\n", tests_passed);
    printf("Failed : %u\n", tests_failed);

    printf("==================================\n");
}

void test_wait_ticks(uint32_t ticks)
{
    uint32_t target;

    target = test_ticks + ticks;

    while (test_ticks < target)
        __asm__ volatile ("pause");
}
