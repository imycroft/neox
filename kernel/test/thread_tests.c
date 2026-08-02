#include "test.h"
#include "thread.h"
#include "process.h"
#include "printf.h"

#define THREAD_TEST_COUNT 32

/* Test functions */

static void dummy_entry(void)
{
    /* Never actually invoked directly by these unit tests. */
}

/*
 * Verify:
 *
 * - thread_create() succeeds.
 * - A valid pointer is returned.
 *
 * This validates:
 * - Basic thread allocation.
 */
static void test_thread_create(void)
{
    struct thread *thread;

    thread = thread_create(NULL, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    test_pass();
}

/*
 * Verify:
 *
 * - A freshly created thread starts in THREAD_READY.
 *
 * This validates:
 * - Initial thread state.
 */
static void test_thread_initial_state(void)
{
    struct thread *thread;

    thread = thread_create(NULL, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    TEST_ASSERT_EQ(thread->state, THREAD_READY);

    test_pass();
}

/*
 * Verify:
 *
 * - A kernel stack of THREAD_STACK_SIZE is allocated.
 * - The saved stack pointer lies within that stack.
 *
 * This validates:
 * - Stack allocation and initial context placement.
 */
static void test_thread_stack_allocated(void)
{
    struct thread *thread;
    uintptr_t stack_start;
    uintptr_t stack_end;

    thread = thread_create(NULL, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    TEST_ASSERT_NOT_NULL(thread->kernel_stack);

    stack_start = (uintptr_t)thread->kernel_stack;
    stack_end = stack_start + THREAD_STACK_SIZE;

    TEST_ASSERT_TRUE(thread->kernel_sp >= stack_start);
    TEST_ASSERT_TRUE(thread->kernel_sp < stack_end);

    test_pass();
}

/*
 * Verify:
 *
 * - The entry point passed to thread_create() is stored.
 *
 * This validates:
 * - Entry point assignment.
 */
static void test_thread_entry_point(void)
{
    struct thread *thread;

    thread = thread_create(NULL, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    TEST_ASSERT_EQ(thread->entry, dummy_entry);

    test_pass();
}

/*
 * Verify:
 *
 * - The owning process passed to thread_create() is stored.
 *
 * This validates:
 * - Process association.
 */
static void test_thread_process_association(void)
{
    struct process *process;
    struct thread *thread;

    process = process_create();

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    TEST_ASSERT_EQ(thread->process, process);

    test_pass();
}

/*
 * Verify:
 *
 * - Every created thread is assigned a unique tid.
 * - TIDs are assigned in increasing order.
 * - No thread is ever assigned tid 0 (reserved for idle).
 *
 * This validates:
 * - TID allocation correctness.
 */
static void test_thread_unique_tids(void)
{
    struct thread *threads[THREAD_TEST_COUNT];
    uint32_t i;

    for (i = 0; i < THREAD_TEST_COUNT; i++)
    {
        threads[i] = thread_create(NULL, dummy_entry);

        TEST_ASSERT_NOT_NULL(threads[i]);

        TEST_ASSERT_TRUE(threads[i]->tid > 0);
    }

    for (i = 1; i < THREAD_TEST_COUNT; i++)
    {
        TEST_ASSERT_TRUE(
            threads[i]->tid > threads[i - 1]->tid
        );
    }

    test_pass();
}

// API

static test_entry_t tests[] =
{
    { "thread_create",              test_thread_create              },
    { "thread_initial_state",       test_thread_initial_state       },
    { "thread_stack_allocated",     test_thread_stack_allocated     },
    { "thread_entry_point",         test_thread_entry_point         },
    { "thread_process_association", test_thread_process_association },
    { "thread_unique_tids",         test_thread_unique_tids         },
};

void test_thread(void)
{
    uint32_t i;

    test_begin("Thread");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
