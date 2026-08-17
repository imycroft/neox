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
#include "string.h"
#include "printf.h"

static volatile bool thread_started;
static volatile bool thread_executed;


static volatile bool thread_a_started;
static volatile bool thread_b_started;

static volatile uint32_t unblock_stage;
static struct thread *blocked_thread;

static volatile bool thread_wait_executed;

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
    struct thread stub;
    interrupt_state_t state;

    state = interrupt_save();

    struct thread *me = scheduler_current();

    scheduler_init();

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    /* Stack-allocated stub — no heap, no cleanup needed */
    memset(&stub, 0, sizeof(stub));
    list_node_init(&stub.group_node);
    list_node_init(&stub.sched_node);
    list_node_init(&stub.wait_node);
    wait_queue_init(&stub.termination_queue);
    stub.state  = THREAD_READY;
    stub.process = process;

    list_push_back(&process->threads, &stub.group_node);
    scheduler_add(&stub);

    TEST_ASSERT_EQ(list_front(&process->threads), &stub.group_node);
    TEST_ASSERT_EQ(scheduler_current(), NULL);
    TEST_ASSERT_EQ(scheduler_next(), &stub);
    TEST_ASSERT_EQ(scheduler_current(), &stub);
    TEST_ASSERT_EQ(stub.state, THREAD_RUNNING);

    scheduler_restore(me);

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

    /*
     * Wait for the worker to fully terminate and free it.
     * yield_entry() yields once then returns, so the worker
     * needs a second scheduling pass to hit thread_exit().
     * thread_join() waits then destroys; thread is invalid
     * after this point.
     */

    thread_join(thread);

    TEST_ASSERT_TRUE(thread_executed);

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
     * Assert state while the pointer is still valid, then
     * join to free the thread struct and its stack.
     */
    TEST_ASSERT_NE(scheduler_current(),
                   thread);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_TERMINATED);

    thread_join(thread);

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

    /*
     * Wait for both workers to fully terminate and free them.
     * Each yields once then returns, so each needs a second
     * scheduling pass to reach thread_exit().
     * Pointers are invalid after thread_join() returns.
     */
    thread_join(thread_a);
    thread_join(thread_b);

    TEST_ASSERT_TRUE(thread_a_started);
    TEST_ASSERT_TRUE(thread_b_started);

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

    TEST_ASSERT_NE(scheduler_current(), thread);

    /*
     * The thread is blocked forever — unblock it so it can
     * run to completion, then join to free it.
     */
    state = interrupt_save();
    thread_unblock(thread);
    interrupt_restore(state);

    thread_join(thread);

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

    TEST_ASSERT_NE(scheduler_current(), thread);

    /*
     * Wake the blocked thread.
     */
    state = interrupt_save();

    thread_unblock(thread);

    interrupt_restore(state);

    TEST_ASSERT_EQ(thread->state,
                   THREAD_READY);

    /*
     * Run it again and wait for it to fully terminate.
     * thread_join() destroys the thread; pointer is invalid
     * after this point.
     */
    thread_join(thread);

    TEST_ASSERT_EQ(unblock_stage, 2);

    test_pass();
}

/*
 * Verify:
 *
 * - thread_join() blocks until the target thread terminates.
 * - The joining thread resumes after the worker exits.
 * - The worker's memory is freed; the pointer is invalid after join.
 *
 * This validates:
 * - Thread termination synchronization.
 * - wait_queue_sleep()/wait_queue_wake_all() integration.
 * - thread_join() wait-and-destroy semantics.
 */
static void test_thread_join(void)
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

    /*
     * thread_join() waits for termination then frees the thread.
     * The pointer is invalid after this returns.
     */
    thread_join(thread);

    TEST_ASSERT_TRUE(thread_wait_executed);

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
    { "thread_join",            test_thread_join             },

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
