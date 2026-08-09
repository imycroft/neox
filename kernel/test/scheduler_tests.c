#include "arch.h"
#include "process.h"
#include "test.h"
#include "scheduler.h"
#include "thread.h"
#include "string.h"
#include "printf.h"
#include "util.h"

/*
 * These tests exercise scheduler_add(), scheduler_next(), and
 * scheduler_current() directly, using plain zeroed struct thread
 * stubs rather than thread_create()'d threads. None of these
 * three functions ever perform a context switch (only
 * scheduler_tick() and scheduler_yield() do that), so it is safe
 * to run them against stub threads that were never given a real
 * stack or entry point.
 *
 * scheduler_init() resets the module's ready list and current
 * thread.  Because the scheduler is a live singleton, each test
 * that calls scheduler_init() must ensure the scheduler is left
 * in a valid state before returning.  test_scheduler() calls
 * scheduler_reset() at the end of the suite to restore the idle
 * thread so the init thread (which is running these tests) can
 * continue executing after the suite returns.  Do not remove
 * this restoration.
 */

static void make_stub(struct thread *thread, uint32_t tid)
{
    memset(thread, 0, sizeof(*thread));

    thread->tid = tid;
    thread->state = THREAD_READY;

    list_node_init(&thread->sched_node);
}
/* Test functions */

/*
 * Verify:
 *
 * - scheduler_next() returns NULL when no thread has been added.
 * - scheduler_current() remains NULL.
 *
 * This validates:
 * - Correct behavior of an empty ready list.
 */
static void test_scheduler_empty(void)
{
    scheduler_init();

    struct thread *next = scheduler_next();

    /* Empty list falls back to idle — never returns NULL */
    TEST_ASSERT_NOT_NULL(next);
    TEST_ASSERT_EQ(next->tid, 0);
    TEST_ASSERT_EQ(scheduler_current()->tid, 0);

    test_pass();
}

/*
 * Verify:
 *
 * - scheduler_add(NULL) is safely ignored.
 *
 * This validates:
 * - NULL-safety of scheduler_add().
 */
static void test_scheduler_add_null(void)
{
    scheduler_init();

    scheduler_add(NULL);

    struct thread *next = scheduler_next();

    /* NULL add is ignored, list still empty, falls back to idle */
    TEST_ASSERT_NOT_NULL(next);
    TEST_ASSERT_EQ(next->tid, 0);

    test_pass();
}

/*
 * Verify:
 *
 * - The first thread added becomes current on the first
 *   scheduler_next() call.
 * - Its state becomes THREAD_RUNNING.
 *
 * This validates:
 * - Ready queue bootstrap behavior.
 */
static void test_scheduler_add_single(void)
{
    struct thread a;
    struct thread *next;

    make_stub(&a, 101);

    scheduler_init();
    scheduler_add(&a);

    next = scheduler_next();

    TEST_ASSERT_EQ(next, &a);
    TEST_ASSERT_EQ(scheduler_current(), &a);
    TEST_ASSERT_EQ(a.state, THREAD_RUNNING);

    test_pass();
}

/*
 * Verify:
 *
 * - Threads are selected in the order they were added.
 * - Selection wraps around to the first thread after the last.
 * - scheduler_current() always matches the selected thread.
 * - Running/ready states are updated correctly.
 *
 * This validates:
 * - Round-robin ready queue ordering.
 * - Current thread tracking.
 * - Thread state transitions during scheduling.
 */
static void test_scheduler_round_robin(void)
{
    struct thread a;
    struct thread b;
    struct thread c;

    make_stub(&a, 102);
    make_stub(&b, 103);
    make_stub(&c, 104);

    scheduler_init();

    scheduler_add(&a);
    scheduler_add(&b);
    scheduler_add(&c);

    TEST_ASSERT_EQ(scheduler_next(), &a);
    TEST_ASSERT_EQ(scheduler_current(), &a);

    TEST_ASSERT_EQ(a.state, THREAD_RUNNING);
    TEST_ASSERT_EQ(b.state, THREAD_READY);
    TEST_ASSERT_EQ(c.state, THREAD_READY);

    TEST_ASSERT_EQ(scheduler_next(), &b);
    TEST_ASSERT_EQ(scheduler_current(), &b);

    TEST_ASSERT_EQ(a.state, THREAD_READY);
    TEST_ASSERT_EQ(b.state, THREAD_RUNNING);
    TEST_ASSERT_EQ(c.state, THREAD_READY);

    TEST_ASSERT_EQ(scheduler_next(), &c);
    TEST_ASSERT_EQ(scheduler_current(), &c);

    TEST_ASSERT_EQ(a.state, THREAD_READY);
    TEST_ASSERT_EQ(b.state, THREAD_READY);
    TEST_ASSERT_EQ(c.state, THREAD_RUNNING);

    TEST_ASSERT_EQ(scheduler_next(), &a);
    TEST_ASSERT_EQ(scheduler_current(), &a);

    TEST_ASSERT_EQ(a.state, THREAD_RUNNING);
    TEST_ASSERT_EQ(b.state, THREAD_READY);
    TEST_ASSERT_EQ(c.state, THREAD_READY);

    TEST_ASSERT_EQ(scheduler_next(), &b);
    TEST_ASSERT_EQ(scheduler_current(), &b);

    TEST_ASSERT_EQ(a.state, THREAD_READY);
    TEST_ASSERT_EQ(b.state, THREAD_RUNNING);
    TEST_ASSERT_EQ(c.state, THREAD_READY);

    test_pass();
}

/*
 * Verify:
 *
 * - Advancing to the next thread demotes the previous
 *   current thread from THREAD_RUNNING to THREAD_READY.
 *
 * This validates:
 * - Thread state bookkeeping across a selection change.
 */
static void test_scheduler_state_transitions(void)
{
    struct thread a, b;

    make_stub(&a, 105);
    make_stub(&b, 106);

    scheduler_init();

    scheduler_add(&a);
    scheduler_add(&b);

    scheduler_next();

    TEST_ASSERT_EQ(a.state, THREAD_RUNNING);

    scheduler_next();

    TEST_ASSERT_EQ(a.state, THREAD_READY);
    TEST_ASSERT_EQ(b.state, THREAD_RUNNING);

    test_pass();
}

/*
 * Verify:
 *
 * - With a single thread in the ready list, repeated calls
 *   to scheduler_next() keep selecting that same thread.
 *
 * This validates:
 * - Round-robin correctness in the degenerate single-thread
 *   case, which is also what the idle thread relies on when
 *   it is the only ready thread.
 */
static void test_scheduler_single_thread_repeats(void)
{
    struct thread a;

    make_stub(&a, 107);

    scheduler_init();
    scheduler_add(&a);

    TEST_ASSERT_EQ(scheduler_next(), &a);
    TEST_ASSERT_EQ(scheduler_next(), &a);
    TEST_ASSERT_EQ(scheduler_next(), &a);

    test_pass();
}

/*
 * Verify:
 *
 * - A current thread can be removed from the ready queue.
 * - scheduler_next() continues from another available thread.
 *
 * This validates:
 * - Scheduler behavior when the current thread is no longer
 *   linked in the ready queue.
 */
static void test_scheduler_remove_ready_thread(void)
{
    struct thread a, b;
    struct thread *next;

    make_stub(&a, 108);
    make_stub(&b, 109);

    scheduler_init();

    scheduler_add(&a);
    scheduler_add(&b);

    next = scheduler_next();

    TEST_ASSERT_EQ(next, &a);

    scheduler_remove(&b);

    next = scheduler_next();

    TEST_ASSERT_EQ(next, &a);

    test_pass();
}

static void test_scheduler_block_current(void)
{
    struct thread a;
    struct thread b;
    struct thread *next;

    make_stub(&a, 1);
    make_stub(&b, 2);

    scheduler_init();

    scheduler_add(&a);
    scheduler_add(&b);

    next = scheduler_next();

    TEST_ASSERT_EQ(next, &a);

    a.state = THREAD_BLOCKED;

    scheduler_remove(&a);

    next = scheduler_next();

    TEST_ASSERT_EQ(next, &b);
    TEST_ASSERT_EQ(b.state, THREAD_RUNNING);

    test_pass();
}

static void test_scheduler_remove_current(void)
{
    struct thread a;
    struct thread b;

    make_stub(&a, 110);
    make_stub(&b, 111);

    scheduler_init();

    scheduler_add(&a);
    scheduler_add(&b);

    TEST_ASSERT_EQ(scheduler_next(), &a);

    scheduler_remove(&a);

    TEST_ASSERT_EQ(scheduler_next(), &b);
    TEST_ASSERT_EQ(scheduler_current(), &b);
    TEST_ASSERT_EQ(b.state, THREAD_RUNNING);

    scheduler_remove(&b);
        /* Empty list falls back to idle — never returns NULL */
    TEST_ASSERT_NOT_NULL(scheduler_next());
    TEST_ASSERT_EQ(scheduler_current()->tid, 0);

    test_pass();
}


// API

static test_entry_t tests[] =
{
    { "scheduler_empty",                 test_scheduler_empty                 },
    { "scheduler_add_null",              test_scheduler_add_null              },
    { "scheduler_add_single",            test_scheduler_add_single            },
    { "scheduler_round_robin",           test_scheduler_round_robin           },
    { "scheduler_state_transitions",     test_scheduler_state_transitions     },
    { "scheduler_single_thread_repeats", test_scheduler_single_thread_repeats },
    { "scheduler_remove_ready_thread",   test_scheduler_remove_ready_thread   },
    { "scheduler_block_current",         test_scheduler_block_current         },
    { "scheduler_remove_current",        test_scheduler_remove_current        },
};

void test_scheduler(void)
{
    uint32_t i;
    test_begin("Scheduler");
    interrupt_state_t state = interrupt_save();
    struct thread *me = scheduler_current();

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);
        tests[i].func();
    }

    scheduler_restore(me);   /* one call, no idle rebuild, no kmalloc */   
    interrupt_restore(state);
    test_end();
}
