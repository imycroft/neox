#include "test.h"
#include "process.h"
#include "printf.h"

#define PROCESS_TEST_COUNT 32

/* Test functions */

/*
 * Verify:
 *
 * - process_create() succeeds.
 * - A valid pointer is returned.
 * - The assigned pid is nonzero.
 *
 * This validates:
 * - Basic process allocation.
 *
 * Note: the pid counter is a persistent, monotonically
 * increasing global, so a specific numeric pid is never
 * asserted here; only that pid 0 (reserved/invalid) is
 * never handed out.
 */
static void test_process_create(void)
{
    struct process *process;

    process = process_create();

    TEST_ASSERT_NOT_NULL(process);

    TEST_ASSERT_TRUE(process->pid > 0);

    test_pass();
}

/*
 * Verify:
 *
 * - Every created process is assigned a unique pid.
 * - PIDs are assigned in increasing order.
 *
 * This validates:
 * - PID allocation correctness.
 */
static void test_process_unique_pids(void)
{
    struct process *processes[PROCESS_TEST_COUNT];
    uint32_t i;

    for (i = 0; i < PROCESS_TEST_COUNT; i++)
    {
        processes[i] = process_create();

        TEST_ASSERT_NOT_NULL(processes[i]);
    }

    for (i = 1; i < PROCESS_TEST_COUNT; i++)
    {
        TEST_ASSERT_TRUE(
            processes[i]->pid > processes[i - 1]->pid
        );
    }

    test_pass();
}

/*
 * Verify:
 *
 * - A freshly created process owns no threads.
 *
 * This validates:
 * - Process initialization zeroes its thread list.
 */
static void test_process_no_threads_initially(void)
{
    struct process *process;

    process = process_create();

    TEST_ASSERT_NOT_NULL(process);

    TEST_ASSERT_NULL(process->threads);

    test_pass();
}

// API

static test_entry_t tests[] =
{
    { "process_create",               test_process_create               },
    { "process_unique_pids",          test_process_unique_pids          },
    { "process_no_threads_initially", test_process_no_threads_initially },
};

void test_process(void)
{
    uint32_t i;

    test_begin("Process");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
