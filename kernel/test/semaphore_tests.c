#include "arch.h"
#include "test.h"
#include "tests.h"

#include "process.h"
#include "thread.h"
#include "scheduler.h"
#include "semaphore.h"
#include "util.h"

static void dummy_entry(void)
{
}

static void test_semaphore_init(void)
{
    struct semaphore sem;

    semaphore_init(&sem, 5);

    TEST_ASSERT_EQ(sem.count, 5);

    TEST_ASSERT_TRUE(list_empty(&sem.wait_queue.threads));

    test_pass();
}

static void test_semaphore_acquire(void)
{
    struct semaphore sem;

    semaphore_init(&sem, 2);

    semaphore_acquire(&sem);

    TEST_ASSERT_EQ(sem.count, 1);

    test_pass();
}

static void test_semaphore_release(void)
{
    struct semaphore sem;

    semaphore_init(&sem, 0);

    semaphore_release(&sem);

    TEST_ASSERT_EQ(sem.count, 1);

    test_pass();
}

static void test_semaphore_release_wakes(void)
{
    struct semaphore sem;
    struct process *process;
    struct thread *thread;
    interrupt_state_t state;

    state = interrupt_save();

    scheduler_init();

    semaphore_init(&sem, 0);

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);
    TEST_ASSERT_NOT_NULL(thread);

    thread->state = THREAD_BLOCKED;

    wait_queue_add(&sem.wait_queue, thread);

    TEST_ASSERT_EQ(thread->wait_queue, &sem.wait_queue);

    semaphore_release(&sem);

    TEST_ASSERT_EQ(thread->state, THREAD_READY);
    TEST_ASSERT_NULL(thread->wait_queue);
    TEST_ASSERT_EQ(sem.count, 0);

    interrupt_restore(state);

    test_pass();
}

// Test Runner

static test_entry_t tests[] =
{
    { "semaphore_init",             test_semaphore_init           },
    { "semaphore_acquire",          test_semaphore_acquire        },
    { "semaphore_release",          test_semaphore_release        },
    { "semaphore_release_wakes",    test_semaphore_release_wakes  },
};

void test_semaphore(void)
{
    uint32_t i;

    test_begin("SEMAPHORE");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
