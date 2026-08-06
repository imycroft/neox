#include "arch.h"
#include "test.h"
#include "tests.h"

#include "mutex.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "util.h"

static void dummy_entry(void)
{
}

static void test_mutex_init(void)
{
    struct mutex mutex;

    mutex_init(&mutex);

    TEST_ASSERT_NULL(mutex.owner);
    TEST_ASSERT_TRUE(list_empty(&mutex.wait_queue.threads));

    test_pass();
}

static void test_mutex_lock(void)
{
    struct mutex mutex;
    struct process *process;
    struct thread *thread;

    scheduler_init();

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);
    TEST_ASSERT_NOT_NULL(thread);

    interrupt_state_t state;

    state = interrupt_save();

    scheduler_add(thread);

    scheduler_next();

    interrupt_restore(state);

    mutex_init(&mutex);

    mutex_lock(&mutex);

    TEST_ASSERT_EQ(mutex.owner, thread);

    test_pass();
}

static void test_mutex_unlock(void)
{
    struct mutex mutex;
    struct process *process;
    struct thread *thread;

    scheduler_init();

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);
    TEST_ASSERT_NOT_NULL(thread);

    interrupt_state_t state;

    state = interrupt_save();

    scheduler_add(thread);

    scheduler_next();

    interrupt_restore(state);

    mutex_init(&mutex);

    mutex_lock(&mutex);

    TEST_ASSERT_EQ(mutex.owner, thread);

    mutex_unlock(&mutex);

    TEST_ASSERT_NULL(mutex.owner);

    test_pass();
}

static void test_mutex_unlock_transfers_owner(void)
{
    struct mutex mutex;
    struct process *process;
    struct thread *owner;
    struct thread *waiter;

    scheduler_init();

    process = process_create("test");
    TEST_ASSERT_NOT_NULL(process);

    owner = thread_create(process, dummy_entry);
    TEST_ASSERT_NOT_NULL(owner);

    waiter = thread_create(process, dummy_entry);
    TEST_ASSERT_NOT_NULL(waiter);

    interrupt_state_t state;

    state = interrupt_save();

    scheduler_add(owner);

    scheduler_next();

    interrupt_restore(state);

    mutex_init(&mutex);

    mutex_lock(&mutex);

    waiter->state = THREAD_BLOCKED;

    wait_queue_add(&mutex.wait_queue, waiter);

    mutex_unlock(&mutex);

    TEST_ASSERT_EQ(mutex.owner, waiter);
    TEST_ASSERT_EQ(waiter->state, THREAD_READY);
    TEST_ASSERT_NULL(waiter->wait_queue);

    test_pass();
}

// Test Runner

static test_entry_t tests[] =
{
    { "mutex_init",                   test_mutex_init                  },
    { "mutex_lock",                   test_mutex_lock                  },
    { "mutex_unlock",                 test_mutex_unlock                },
    { "mutex_unlock_transfers_owner", test_mutex_unlock_transfers_owner},

};

void test_mutex(void)
{
    uint32_t i;

    test_begin("MUTEX");

    for (i = 0; i < (sizeof(tests) / sizeof((tests)[0])); i++) {
      test_case(tests[i].name);

      tests[i].func();
    }

    test_end();
}
