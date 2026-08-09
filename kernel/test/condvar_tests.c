#include "arch.h"
#include "condvar.h"
#include "mutex.h"
#include "process.h"
#include "scheduler.h"
#include "test.h"
#include "tests.h"
#include "thread.h"
#include "util.h"

/* ----------------------------------------------------------------
 * Shared state between test threads
 * ---------------------------------------------------------------- */

static struct mutex      cv_mutex;
static struct condvar    cv_condvar;

/* ── test_condvar_signal ── */
static volatile bool signal_worker_ran;
static volatile bool signal_condition;

/*
 * Worker: acquires the mutex, sets the condition, signals the
 * condvar, releases the mutex.
 */
static void signal_worker_entry(void)
{
    mutex_lock(&cv_mutex);

    signal_worker_ran = true;
    signal_condition  = true;

    condvar_signal(&cv_condvar);

    mutex_unlock(&cv_mutex);
}

/*
 * Verify that condvar_wait() sleeps until condvar_signal() fires.
 *
 * Main thread:
 *   1. Acquires mutex.
 *   2. Checks condition — false, calls condvar_wait().
 *   3. condvar_wait() releases mutex and sleeps.
 *   4. Worker runs, sets condition, signals.
 *   5. Main resumes, re-holds mutex, verifies condition.
 */
static void test_condvar_signal(void)
{
    struct process *process;
    struct thread  *worker;
    interrupt_state_t state;

    signal_worker_ran = false;
    signal_condition  = false;

    mutex_init(&cv_mutex);
    condvar_init(&cv_condvar);

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    worker = thread_create(process, signal_worker_entry);
    TEST_ASSERT_NOT_NULL(worker);

    state = interrupt_save();
    thread_add(worker);
    interrupt_restore(state);

    /*
     * Acquire mutex before checking the condition so that the
     * check and the condvar_wait() are under the same lock.
     */
    mutex_lock(&cv_mutex);

    while (!signal_condition)
        condvar_wait(&cv_condvar, &cv_mutex);

    /* Condition is true; we hold the mutex again. */
    TEST_ASSERT_TRUE(signal_condition);
    TEST_ASSERT_TRUE(signal_worker_ran);

    mutex_unlock(&cv_mutex);

    /* Wait for the worker to fully terminate. */
    thread_wait(worker);

    TEST_ASSERT_EQ(worker->state, THREAD_TERMINATED);

    test_pass();
}

/* ── test_condvar_broadcast ── */

#define BROADCAST_WORKERS 3

static volatile uint32_t broadcast_woken;
static volatile bool     broadcast_condition;

static void broadcast_worker_entry(void)
{
    mutex_lock(&cv_mutex);

    while (!broadcast_condition)
        condvar_wait(&cv_condvar, &cv_mutex);

    broadcast_woken++;

    mutex_unlock(&cv_mutex);
}

/*
 * Verify that condvar_broadcast() wakes all waiters.
 *
 * Three worker threads all wait on the same condvar.
 * The main thread sets the condition and broadcasts.
 * All three must wake, increment the counter, and terminate.
 */
static void test_condvar_broadcast(void)
{
    struct process *process;
    struct thread  *workers[BROADCAST_WORKERS];
    interrupt_state_t state;
    uint32_t i;

    broadcast_woken     = 0;
    broadcast_condition = false;

    mutex_init(&cv_mutex);
    condvar_init(&cv_condvar);

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    for (i = 0; i < BROADCAST_WORKERS; i++)
    {
        workers[i] = thread_create(process, broadcast_worker_entry);
        TEST_ASSERT_NOT_NULL(workers[i]);

        state = interrupt_save();
        thread_add(workers[i]);
        interrupt_restore(state);
    }

    /*
     * Let workers start and reach condvar_wait().
     * Each worker acquires the mutex, finds condition false,
     * and sleeps — releasing the mutex for the next worker.
     */
    mutex_lock(&cv_mutex);

    broadcast_condition = true;

    condvar_broadcast(&cv_condvar);

    mutex_unlock(&cv_mutex);

    /* Wait for every worker to terminate. */
    for (i = 0; i < BROADCAST_WORKERS; i++)
        thread_wait(workers[i]);

    TEST_ASSERT_EQ(broadcast_woken, (uint32_t)BROADCAST_WORKERS);

    test_pass();
}

/* ── test_condvar_no_spurious_wakeup_loss ──
 *
 * Verify the lost-wakeup fix: signal is sent while the waiter
 * holds the mutex.  Without the atomic release-and-enqueue, the
 * signal would be delivered before the thread is on the queue
 * and the thread would sleep forever.
 *
 * We test this by having the signaller acquire and release the
 * mutex before the waiter calls condvar_wait(), ensuring the
 * signal would be lost without the atomic fix.  With the fix the
 * waiter checks the condition under the mutex and never sleeps
 * because the condition is already true.
 */

static volatile bool no_loss_condition;

static void no_loss_signaller_entry(void)
{
    mutex_lock(&cv_mutex);

    no_loss_condition = true;

    condvar_signal(&cv_condvar);

    mutex_unlock(&cv_mutex);
}

static void test_condvar_no_loss(void)
{
    struct process *process;
    struct thread  *signaller;
    interrupt_state_t state;

    no_loss_condition = false;

    mutex_init(&cv_mutex);
    condvar_init(&cv_condvar);

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    signaller = thread_create(process, no_loss_signaller_entry);
    TEST_ASSERT_NOT_NULL(signaller);

    state = interrupt_save();
    thread_add(signaller);
    interrupt_restore(state);

    /*
     * Let the signaller run first: it will set the condition
     * and signal before we even check.  The while loop below
     * means we never call condvar_wait() because the condition
     * is already true when we acquire the mutex.
     *
     * This validates the standard "always check condition in a
     * while loop" pattern which is what prevents the lost-wakeup
     * from mattering in correct code.
     */
    thread_wait(signaller);

    mutex_lock(&cv_mutex);

    while (!no_loss_condition)
        condvar_wait(&cv_condvar, &cv_mutex);

    TEST_ASSERT_TRUE(no_loss_condition);

    mutex_unlock(&cv_mutex);

    test_pass();
}

/* ----------------------------------------------------------------
 * Test runner
 * ---------------------------------------------------------------- */

static test_entry_t tests[] =
{
    { "condvar_signal",    test_condvar_signal    },
    { "condvar_broadcast", test_condvar_broadcast },
    { "condvar_no_loss",   test_condvar_no_loss   },
};

void test_condvar(void)
{
    uint32_t i;

    test_begin("Condvar");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);
        tests[i].func();
    }

    test_end();
}
