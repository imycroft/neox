#include "process.h"
#include "scheduler.h"
#include "tests.h"
#include "test.h"

#include "util.h"
#include "wait.h"
#include "thread.h"

static void dummy_entry(void)
{
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

static void test_wait_queue_add_remove(void)
{
    struct wait_queue queue;
    struct thread a;
    struct thread b;
    struct thread *thread;


    list_node_init(&a.wait_node);
    list_node_init(&b.wait_node);

    wait_queue_init(&queue);

    wait_queue_add(&queue, &a);
    wait_queue_add(&queue, &b);


    thread = wait_queue_remove(&queue);

    TEST_ASSERT_EQ(thread, &a);


    thread = wait_queue_remove(&queue);

    TEST_ASSERT_EQ(thread, &b);


    thread = wait_queue_remove(&queue);

    TEST_ASSERT_NULL(thread);


    test_pass();
}

static void test_wait_queue_wake(void)
{
    struct process *process;
    struct thread *thread;
    struct wait_queue queue;

    scheduler_init();

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    thread = thread_create(process, dummy_entry);

    TEST_ASSERT_NOT_NULL(thread);

    thread->state = THREAD_BLOCKED;

    wait_queue_init(&queue);

    wait_queue_add(&queue, thread);

    TEST_ASSERT_EQ(thread->wait_queue, &queue);

    wait_queue_wake(&queue);

    TEST_ASSERT_EQ(thread->state, THREAD_READY);

    TEST_ASSERT_NULL(thread->wait_queue);

    test_pass();
}

static void test_wait_queue_wake_one(void)
{
    struct wait_queue queue;
    struct process *process;
    struct thread *a;
    struct thread *b;

    scheduler_init();

    wait_queue_init(&queue);

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    a = thread_create(process, dummy_entry);
    b = thread_create(process, dummy_entry);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);

    a->state = THREAD_BLOCKED;
    b->state = THREAD_BLOCKED;

    wait_queue_add(&queue, a);
    wait_queue_add(&queue, b);

    wait_queue_wake(&queue);

    TEST_ASSERT_EQ(a->state, THREAD_READY);
    TEST_ASSERT_EQ(b->state, THREAD_BLOCKED);

    TEST_ASSERT_NULL(a->wait_queue);
    TEST_ASSERT_EQ(b->wait_queue, &queue);

    test_pass();
}

static void test_wait_queue_wake_all(void)
{
    struct wait_queue queue;
    struct process *process;
    struct thread *a;
    struct thread *b;
    struct thread *c;

    scheduler_init();

    wait_queue_init(&queue);

    process = process_create("test");

    TEST_ASSERT_NOT_NULL(process);

    a = thread_create(process, dummy_entry);
    b = thread_create(process, dummy_entry);
    c = thread_create(process, dummy_entry);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    a->state = THREAD_BLOCKED;
    b->state = THREAD_BLOCKED;
    c->state = THREAD_BLOCKED;

    wait_queue_add(&queue, a);
    wait_queue_add(&queue, b);
    wait_queue_add(&queue, c);

    wait_queue_wake_all(&queue);

    TEST_ASSERT_EQ(a->state, THREAD_READY);
    TEST_ASSERT_EQ(b->state, THREAD_READY);
    TEST_ASSERT_EQ(c->state, THREAD_READY);

    TEST_ASSERT_NULL(a->wait_queue);
    TEST_ASSERT_NULL(b->wait_queue);
    TEST_ASSERT_NULL(c->wait_queue);

    TEST_ASSERT_TRUE(list_empty(&queue.threads));

    test_pass();
}

// Start
static test_entry_t tests[] =
{
    { "wait_queue_add_remove", test_wait_queue_add_remove },
    { "wait_queue_wake",       test_wait_queue_wake       },
    { "wait_queue_wake_one",   test_wait_queue_wake_one   },
    { "wait_queue_wake_all",   test_wait_queue_wake_all   },


};

void test_wait(void)
{
    uint32_t i;

    test_begin("Wait");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
