/*
 * These tests exercise the interaction between the thread
 * subsystem and the scheduler using real threads created by
 * thread_create(). Unlike scheduler_test.c, these tests never
 * manipulate scheduler internals directly.
 *
 * Only the public thread API is used.
 */

/*
 * Verify:
 *
 * - thread_add() inserts the thread into its owning process.
 * - thread_add() makes the thread runnable.
 * - The scheduler can immediately select the thread.
 *
 * This validates:
 * - Integration between the thread, process, and scheduler
 *   subsystems using only the public thread API.
 */

#include "arch.h"
#include "process.h"
#include "scheduler.h"
#include "test.h"
#include "util.h"

static volatile bool thread_started;
static volatile bool thread_executed;


static volatile bool thread_a_started;
static volatile bool thread_b_started;

static volatile uint32_t unblock_stage;
static struct thread *blocked_thread;

static volatile bool thread_wait_executed;


static void dummy_entry(void)
{
    return;
}

static void yield_entry(void)
{
    thread_executed = true;

    thread_yield();
}


static void exit_entry(void)
{
    thread_started = true;
}

static void wait_entry(void)
{
    thread_wait_executed = true;
}

static void thread_a_entry(void)
{
    thread_a_started = true;

    thread_yield();
}

static void thread_b_entry(void)
{
    thread_b_started = true;

    thread_yield();
}

static void unblock_entry(void)
{
    unblock_stage = 1;

    blocked_thread = scheduler_current();

    thread_block();

    unblock_stage = 2;
}


// Test functions

static void test_thread_add(void)
{
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    /*
     * This test exercises the raw bootstrap path of
     * scheduler_next() (current == NULL → pick front).
     * It must call scheduler_init() directly rather than
     * scheduler_reset() because reset leaves current pointing
     * at the idle thread, which would fail the NULL assertion
     * below.  State is restored with scheduler_reset() after
     * the assertions so the init thread can continue.
     */
    state = interrupt_save();

    scheduler_init();

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    thread_add(thread);

    TEST_ASSERT_EQ(list_front(&process->threads),
                   &thread->group_node);

    TEST_ASSERT_EQ(scheduler_current(),
                   NULL);

    TEST_ASSERT_EQ(scheduler_next(),
                   thread);

    TEST_ASSERT_EQ(scheduler_current(),
                   thread);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_RUNNING);

    interrupt_restore(state);

    test_pass();
}

/*
 * Verify:
 *
 * - A newly added thread is actually executed.
 * - thread_yield() transfers execution to the runnable thread.
 * - The thread can yield back to the idle thread.
 *
 * This validates:
 * - Context switching.
 * - Thread bootstrap.
 * - Cooperative scheduling.
 */
static void test_thread_execution(void)
{
    struct process *process;
    struct thread *thread;

    interrupt_state_t state;

    thread_executed = false;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           yield_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    thread_yield();

    TEST_ASSERT_TRUE(thread_executed);

    TEST_ASSERT_NE(scheduler_current(), thread);

    test_pass();
}

/*
 * Verify:
 *
 * - A thread starts executing.
 * - Returning from the entry function terminates the thread.
 * - The terminated thread is not scheduled again.
 *
 * This validates:
 * - thread_bootstrap() termination path.
 * - thread_exit() behavior.
 * - Scheduler handling of terminated threads.
 */
static void test_thread_exit(void)
{
    struct process *process;
    struct thread *thread;

    interrupt_state_t state;

    thread_started = false;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           exit_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    /*
     * First switch:
     *
     * idle -> thread
     */
    thread_yield();

    TEST_ASSERT_TRUE(thread_started);

    /*
     * The terminated thread should no longer be current.
     */
    TEST_ASSERT_NE(scheduler_current(),
                   thread);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_TERMINATED);

    test_pass();
}

/*
 * Verify:
 *
 * - Multiple real threads can execute.
 * - A thread can voluntarily yield.
 * - The scheduler switches between different kernel contexts.
 *
 * This validates:
 * - Multiple kernel stacks.
 * - Context preservation.
 * - Cooperative scheduling between real threads.
 */
static void test_multiple_thread_yield(void)
{
    struct process *process;
    struct thread *thread_a;
    struct thread *thread_b;

    interrupt_state_t state;

    thread_a_started = false;
    thread_b_started = false;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread_a = thread_create(process,
                             thread_a_entry);

    TEST_ASSERT_NOT_NULL(thread_a);

    thread_b = thread_create(process,
                             thread_b_entry);

    TEST_ASSERT_NOT_NULL(thread_b);

    state = interrupt_save();

    thread_add(thread_a);
    thread_add(thread_b);

    interrupt_restore(state);

    thread_yield();

    TEST_ASSERT_TRUE(thread_a_started);
    TEST_ASSERT_TRUE(thread_b_started);

    TEST_ASSERT_EQ(scheduler_current()->tid,
                   0);

    test_pass();
}

static volatile bool thread_blocked;

static void block_entry(void)
{
    thread_blocked = true;

    thread_block();
}

/*
 * Verify:
 *
 * - A running thread can block itself.
 * - The blocked thread is removed from the ready queue.
 * - The scheduler falls back to the idle thread.
 *
 * This validates:
 * - thread_block()
 * - Scheduler removal of blocked threads.
 * - Idle thread fallback.
 */
static void test_thread_block(void)
{
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    thread_blocked = false;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           block_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    thread_yield();

    TEST_ASSERT_TRUE(thread_blocked);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_BLOCKED);

    TEST_ASSERT_EQ(scheduler_current()->tid,
                   0);

    test_pass();
}

/*
 * Verify:
 *
 * - A blocked thread can be unblocked.
 * - The unblocked thread becomes runnable again.
 * - The scheduler executes the resumed thread.
 *
 * This validates:
 * - thread_block()
 * - thread_unblock()
 * - Scheduler reinsertion of blocked threads.
 * - Context restoration after blocking.
 */
static void test_thread_unblock_resume(void)
{
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    unblock_stage = 0;
    blocked_thread = NULL;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           unblock_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    /*
     * Run the thread until it blocks.
     */
    thread_yield();

    TEST_ASSERT_EQ(unblock_stage, 1);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_BLOCKED);

    TEST_ASSERT_EQ(scheduler_current()->tid,
                   0);

    /*
     * Wake the blocked thread.
     */
    state = interrupt_save();

    thread_unblock(thread);

    interrupt_restore(state);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_READY);

    /*
     * Run it again.
     */
    thread_yield();

    TEST_ASSERT_EQ(unblock_stage, 2);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_TERMINATED);

    TEST_ASSERT_EQ(scheduler_current()->tid,
                   0);

    test_pass();
}

/*
 * Verify:
 *
 * - thread_wait() blocks until the target thread terminates.
 * - The waiting thread resumes after the worker exits.
 * - The worker is left in the THREAD_TERMINATED state.
 *
 * This validates:
 * - Thread termination synchronization.
 * - wait_queue_sleep()/wait_queue_wake_all() integration.
 * - thread_wait() behavior.
 */
static void test_thread_wait(void)
{
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    thread_wait_executed = false;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           wait_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    thread_wait(thread);

    TEST_ASSERT_TRUE(thread_wait_executed);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_TERMINATED);

    TEST_ASSERT_EQ(scheduler_current()->tid,
                   0);

    test_pass();
}

// API

static test_entry_t tests[] =
{
    { "thread_add",             test_thread_add              },
    { "thread_execution",       test_thread_execution        },
    { "thread_exit",            test_thread_exit             },
    { "multiple_thread_yield",  test_multiple_thread_yield   },
    { "thread_block",           test_thread_block            },
    { "thread_unblock_resume",  test_thread_unblock_resume   },
    { "thread_wait",            test_thread_wait             },

};

void test_thread_integration(void)
{
    uint32_t i;

    test_begin("Thread Integration");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
