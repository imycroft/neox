#include "arch.h"
#include "process.h"
#include "scheduler.h"
#include "test.h"
#include "thread.h"
#include "util.h"
#include "printf.h"

#define PREEMPTION_THREAD_COUNT 3
#define PREEMPTION_STRESS_COUNT 32



static volatile uint32_t counter_c;

static volatile bool stop;
static volatile uint32_t counter;

static volatile uint32_t counter_a;
static volatile uint32_t counter_b;

static volatile uint32_t blocked_counter;
static volatile uint32_t runner_counter;

static volatile uint32_t quantum_thread_a_runs;
static volatile uint32_t quantum_thread_b_runs;

static volatile uint32_t stress_counters[PREEMPTION_STRESS_COUNT];

static struct thread *stress_threads[PREEMPTION_STRESS_COUNT];


struct stress_context
{
    uint32_t index;
};


static void preempt_entry(void)
{
    while (!stop)
        counter++;
}

static void thread_a_entry(void)
{
    while (!stop)
        counter_a++;
}

static void thread_b_entry(void)
{
    while (!stop)
        counter_b++;
}

static void runner_entry(void)
{
    while (!stop)
        runner_counter++;
}

static void blocked_entry(void)
{
    thread_block();

    while (!stop)
        blocked_counter++;
}

static void thread_c_entry(void)
{
    while (!stop)
        counter_c++;
}

static void quantum_thread_a_entry(void)
{
    while (!stop)
        quantum_thread_a_runs++;
}

static void quantum_thread_b_entry(void)
{
    while (!stop)
        quantum_thread_b_runs++;
}

static void stress_entry(void)
{
    uint32_t i;

    while (!stop)
    {
        for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
        {
            if (stress_threads[i] == scheduler_current())
            {
                stress_counters[i]++;
                break;
            }
        }
    }
}

////
static void test_wait_ticks_basic(void)
{
    uint32_t before;

    before = test_ticks;

    test_wait_ticks(5);

    TEST_ASSERT_TRUE(test_ticks >= before + 5);

    test_pass();
}

/*
 * Verify:
 *
 * - A CPU-bound thread that never voluntarily yields
 *   executes under timer-driven preemption.
 * - The thread terminates cleanly after being stopped.
 *
 * This validates:
 * - PIT interrupt delivery.
 * - scheduler_tick() preemption.
 * - Context switching without cooperative yielding.
 */
static void test_timer_preemption(void)
{
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    stop = false;
    counter = 0;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process,
                           preempt_entry);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();

    thread_add(thread);

    interrupt_restore(state);

    /*
     * Give the scheduler time to preempt the idle
     * thread and execute the worker.
     */
    test_wait_ticks(20);

    TEST_ASSERT_TRUE(counter > 0);

    stop = true;

    /*
     * thread_join() waits for termination then frees the thread.
     * Pointer is invalid after this returns.
     */
    thread_join(thread);

    test_pass();
}

/*
 * Verify:
 *
 * - Two CPU-bound threads that never voluntarily yield
 *   both receive processor time.
 * - Both threads terminate cleanly after being stopped.
 *
 * This validates:
 * - Timer-driven round-robin scheduling.
 * - Fair progress of runnable threads.
 * - Repeated preemptive context switching.
 */
/*
 * Verify:
 *
 * - Two CPU-bound threads that never voluntarily yield
 *   both receive processor time.
 * - Both threads terminate cleanly after being stopped.
 *
 * This validates:
 * - Timer-driven round-robin scheduling.
 * - Fair progress of runnable threads.
 * - Repeated preemptive context switching.
 */
static void test_timer_round_robin(void)
{
    struct process *process;
    struct thread *thread_a;
    struct thread *thread_b;
    interrupt_state_t state;

    stop = false;

    counter_a = 0;
    counter_b = 0;

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

    test_wait_ticks(SCHEDULER_QUANTUM_TICKS+1);

    stop = true;

    thread_join(thread_a);
    thread_join(thread_b);

    TEST_ASSERT_TRUE(counter_a > 0);
    TEST_ASSERT_TRUE(counter_b > 0);

    test_pass();
}

/*
 * Verify:
 *
 * - A blocked thread receives no CPU time.
 * - Runnable threads continue executing while another
 *   thread remains blocked.
 * - Unblocking the thread allows it to execute again.
 *
 * This validates:
 * - Scheduler exclusion of blocked threads.
 * - Timer-driven scheduling with blocked threads.
 */
static void test_timer_blocked_thread(void)
{
    struct process *process;
    struct thread *runner;
    struct thread *blocked;
    interrupt_state_t state;

    stop = false;

    runner_counter = 0;
    blocked_counter = 0;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    runner = thread_create(process,
                           runner_entry);

    TEST_ASSERT_NOT_NULL(runner);

    blocked = thread_create(process,
                            blocked_entry);

    TEST_ASSERT_NOT_NULL(blocked);

    state = interrupt_save();

    thread_add(runner);
    thread_add(blocked);

    interrupt_restore(state);

    /*
     * Give the blocked thread time to execute
     * thread_block().
     */
    test_wait_ticks(SCHEDULER_QUANTUM_TICKS+1);

    TEST_ASSERT_EQ(blocked->state,
                   THREAD_BLOCKED);

    TEST_ASSERT_TRUE(runner_counter > 0);

    TEST_ASSERT_EQ(blocked_counter, 0);

    state = interrupt_save();

    thread_unblock(blocked);

    interrupt_restore(state);

    test_wait_ticks(SCHEDULER_QUANTUM_TICKS+1);

    stop = true;

    thread_join(runner);
    thread_join(blocked);

    TEST_ASSERT_TRUE(blocked_counter > 0);

    test_pass();
}

/*
 * Verify:
 *
 * - Multiple CPU-bound threads receive CPU time.
 * - Round-robin scheduling continues correctly with
 *   more than two runnable threads.
 *
 * This validates:
 * - Ready queue wrap-around under timer preemption.
 * - Scheduler fairness with multiple threads.
 */
static void test_timer_multiple_threads(void)
{
    struct process *process;
    struct thread *thread_a;
    struct thread *thread_b;
    struct thread *thread_c;
    interrupt_state_t state;

    stop = false;

    counter_a = 0;
    counter_b = 0;
    counter_c = 0;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread_a = thread_create(process,
                             thread_a_entry);

    TEST_ASSERT_NOT_NULL(thread_a);

    thread_b = thread_create(process,
                             thread_b_entry);

    TEST_ASSERT_NOT_NULL(thread_b);

    thread_c = thread_create(process,
                             thread_c_entry);

    TEST_ASSERT_NOT_NULL(thread_c);

    state = interrupt_save();

    thread_add(thread_a);
    thread_add(thread_b);
    thread_add(thread_c);

    interrupt_restore(state);

    test_wait_ticks(SCHEDULER_QUANTUM_TICKS * 2);

    stop = true;

    thread_join(thread_a);
    thread_join(thread_b);
    thread_join(thread_c);

    TEST_ASSERT_TRUE(counter_a > 0);
    TEST_ASSERT_TRUE(counter_b > 0);
    TEST_ASSERT_TRUE(counter_c > 0);

    test_pass();
}

/*
 * Verify:
 *
 * - scheduler_start() initializes the quantum counter.
 *
 * This validates:
 * - Initial scheduler timing state.
 */
static void test_quantum_initial_value(void)
{
    interrupt_state_t state;

    state = interrupt_save();

    TEST_ASSERT_EQ(
        scheduler_get_quantum_remaining(),
                   SCHEDULER_QUANTUM_TICKS
    );

    interrupt_restore(state);

    test_pass();
}

/*
 * Verify:
 *
 * - The scheduler quantum counter decreases on every timer tick.
 *
 * This validates:
 * - Timer tick accounting.
 * - Quantum countdown logic.
 */
static void test_quantum_countdown(void)
{
    interrupt_state_t state;
    uint32_t remaining;

    state = interrupt_save();

    interrupt_restore(state);

    remaining = scheduler_get_quantum_remaining();

    TEST_ASSERT_EQ(remaining,
                   SCHEDULER_QUANTUM_TICKS);

    test_wait_ticks(1);

    remaining = scheduler_get_quantum_remaining();

    TEST_ASSERT_EQ(remaining,
                   SCHEDULER_QUANTUM_TICKS - 1);

    test_wait_ticks(1);

    remaining = scheduler_get_quantum_remaining();

    TEST_ASSERT_EQ(remaining,
                   SCHEDULER_QUANTUM_TICKS - 2);

    test_pass();
}

/*
 * Verify:
 *
 * - A running thread is preempted after its quantum expires.
 * - The scheduler resets the quantum for the next thread.
 *
 * This validates:
 * - Quantum expiration.
 * - Timer-driven context switching.
 * - Quantum reset after scheduling.
 */
static void test_quantum_reset_after_switch(void)
{
    struct process *process;
    struct thread *thread_a;
    struct thread *thread_b;
    interrupt_state_t state;

    stop = false;

    quantum_thread_a_runs = 0;
    quantum_thread_b_runs = 0;

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread_a = thread_create(process,
                             quantum_thread_a_entry);

    TEST_ASSERT_NOT_NULL(thread_a);

    thread_b = thread_create(process,
                             quantum_thread_b_entry);

    TEST_ASSERT_NOT_NULL(thread_b);

    state = interrupt_save();

    thread_add(thread_a);
    thread_add(thread_b);

    interrupt_restore(state);

    /*
     * Let the first thread consume its quantum.
     */
    test_wait_ticks(SCHEDULER_QUANTUM_TICKS);

    TEST_ASSERT_TRUE(
        quantum_thread_a_runs > 0
    );

    TEST_ASSERT_TRUE(
        quantum_thread_b_runs > 0
    );

    TEST_ASSERT_EQ(
        scheduler_get_quantum_remaining(),
                   SCHEDULER_QUANTUM_TICKS
    );

    stop = true;

    thread_join(thread_a);
    thread_join(thread_b);

    test_pass();
}

/*
 * Verify:
 *
 * - Many CPU-bound threads are scheduled.
 * - No runnable thread is permanently starved.
 *
 * This validates:
 * - Ready queue scalability.
 * - Round-robin behavior under load.
 * - Timer preemption with many threads.
 */
static void test_timer_stress(void)
{
    struct process *process;
    interrupt_state_t state;
    uint32_t i;

    stop = false;

    for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
        stress_counters[i] = 0;


    process = process_create("stress");

    TEST_ASSERT_NOT_NULL(process);


    for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
    {
        stress_threads[i] =
        thread_create(process,
                      stress_entry);

        TEST_ASSERT_NOT_NULL(stress_threads[i]);
    }


    state = interrupt_save();

    for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
        thread_add(stress_threads[i]);

    interrupt_restore(state);


    test_wait_ticks(SCHEDULER_QUANTUM_TICKS *
    PREEMPTION_STRESS_COUNT);


    stop = true;


    for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
        thread_join(stress_threads[i]);


    for (i = 0; i < PREEMPTION_STRESS_COUNT; i++)
    {
        TEST_ASSERT_TRUE(
            stress_counters[i] > 0
        );
    }

    test_pass();
}


// API

static test_entry_t tests[] =
{
    { "wait_ticks_basic",  test_wait_ticks_basic },
    { "timer_preemption",  test_timer_preemption },
    { "timer_round_robin", test_timer_round_robin },
    { "timer_blocked_thread", test_timer_blocked_thread },
    { "timer_multiple_threads", test_timer_multiple_threads },
    { "quantum_initial_value", test_quantum_initial_value },
    { "quantum_countdown", test_quantum_countdown },
    { "quantum_reset_after_switch", test_quantum_reset_after_switch },
    { "timer_stress", test_timer_stress },


};

void test_thread_preemption(void)
{

printf("hello %x\n", tests[0].name);
}
