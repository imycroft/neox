#include "test.h"
#include "printf.h"

volatile uint32_t test_ticks = 0;

static uint32_t tests_passed;
static uint32_t tests_failed;

static const char *current_suite;
static const char *current_test;
static const char *current_assertion;

/*
 * Fixed-size failure log.  Each entry records the suite name,
 * test name, and the assertion expression that triggered the
 * failure.  Sized generously; if MAX_FAILURES is ever hit the
 * summary will say so.
 */
#define MAX_FAILURES 64

typedef struct
{
    const char *suite;
    const char *test;
    const char *assertion;

} test_failure_t;

static test_failure_t failures[MAX_FAILURES];

void test_begin(const char *suite)
{
    current_suite = suite;

    printf("\n===== %s =====\n", suite);
}

void test_end(void)
{
    printf("====================\n\n");
}

void test_pass(void)
{
    tests_passed++;

    printf("    [PASS] %s\n", current_test);
}

void test_fail(void)
{
    printf("    [FAIL] %s\n", current_test);
    printf("           Assertion: %s\n", current_assertion);

    if (tests_failed < MAX_FAILURES)
    {
        failures[tests_failed].suite     = current_suite;
        failures[tests_failed].test      = current_test;
        failures[tests_failed].assertion = current_assertion;
    }

    tests_failed++;
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
    uint32_t i;

    printf("\n========== TEST SUMMARY ==========\n");
    printf("Passed : %u\n", tests_passed);
    printf("Failed : %u\n", tests_failed);

    if (tests_failed > 0)
    {
        printf("\nFailed tests:\n");

        uint32_t reported = tests_failed < MAX_FAILURES
        ? tests_failed
        : MAX_FAILURES;

        for (i = 0; i < reported; i++)
        {
            printf("  [%u] %s :: %s\n",
                   i + 1,
                   failures[i].suite,
                   failures[i].test);

            printf("      Assertion: %s\n",
                   failures[i].assertion);
        }

        if (tests_failed > MAX_FAILURES)
            printf("  ... and %u more (increase MAX_FAILURES)\n",
                   tests_failed - MAX_FAILURES);
    }

    printf("==================================\n");
}

void test_wait_ticks(uint32_t ticks)
{
    uint32_t target;

    target = test_ticks + ticks;

    while (test_ticks < target)
        __asm__ volatile ("pause");
}
