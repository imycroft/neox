#include "wait.h"

#include "assert.h"
#include "thread.h"
#include "util.h"
#include "scheduler.h"
#include "scheduler_internal.h"

#include "arch.h"
void wait_queue_init(struct wait_queue *queue)
{
    ASSERT(queue != NULL);

    list_init(&queue->threads);
}


void wait_queue_add(
    struct wait_queue *queue,
    struct thread *thread
)
{
    ASSERT(queue != NULL);
    ASSERT(thread != NULL);

    thread->wait_queue = queue;

    list_push_back(
        &queue->threads,
        &thread->wait_node
    );
}


struct thread *wait_queue_remove(
    struct wait_queue *queue
)
{
    struct list_node *node;
    struct thread *thread;

    ASSERT(queue != NULL);

    if (list_empty(&queue->threads))
        return NULL;

    node = list_front(&queue->threads);

    list_remove(node);

    thread =
    container_of(
        node,
        struct thread,
        wait_node
    );

    thread->wait_queue = NULL;

    return thread;
}

void wait_queue_wake(struct wait_queue *queue)
{
    struct thread *thread;

    ASSERT(queue != NULL);

    thread = wait_queue_remove(queue);

    if (thread == NULL)
        return;

    thread_unblock(thread);
}

void wait_queue_wake_all(struct wait_queue *queue)
{
    struct thread *thread;

    ASSERT(queue != NULL);

    while ((thread = wait_queue_remove(queue)) != NULL)
        thread_unblock(thread);
}

void wait_queue_sleep(struct wait_queue *queue)
{
    struct thread *thread;
    interrupt_state_t state;

    ASSERT(queue != NULL);

    thread = scheduler_current();

    ASSERT(thread != NULL);

    state = interrupt_save();

    wait_queue_add(queue, thread);

    thread->state = THREAD_BLOCKED;

    scheduler_remove(thread);

    scheduler_yield();

    interrupt_restore(state);
}

