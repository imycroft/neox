#include "scheduler.h"
#include "scheduler_internal.h"
#include "arch.h"
#include "assert.h"
#include "list.h"
#include "thread.h"
#include "string.h"

#include "util.h"

#include "arch/x86/context.h"

static struct list ready_list;
static struct thread *current;

/*
 * Ticks remaining before the current thread is preempted.
 */
static uint32_t quantum_remaining;

/*
 * Represents the execution context the scheduler was started
 * from (the boot stack). It owns no allocated kernel_stack;
 * its kernel_sp is captured the first time it is switched
 * away from.
 */
static struct thread idle_thread;

void scheduler_init(void)
{
    list_init(&ready_list);

    current = NULL;

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;
}

void scheduler_add(struct thread *thread)
{
    if (thread == NULL)
        return;

    list_push_back(&ready_list,
                   &thread->sched_node);
}

void scheduler_remove(struct thread *thread)
{
    ASSERT(thread != NULL);

    list_remove(&thread->sched_node);
}

struct thread *scheduler_current(void)
{
    return current;
}

struct thread *scheduler_next(void)
{
    struct list_node *node;

    if (list_empty(&ready_list))
        return NULL;

    if (current == NULL)
    {
        node = list_front(&ready_list);

        current =
        container_of(node,
                     struct thread,
                     sched_node);

        current->state = THREAD_RUNNING;

        return current;
    }

    if (current->state == THREAD_RUNNING)
        current->state = THREAD_READY;

    if (current->sched_node.next == NULL &&
        current->sched_node.prev == NULL)
    {
        node = list_front(&ready_list);
    }
    else
    {
        node = list_next(&current->sched_node);

        if (node == &ready_list.head)
            node = list_front(&ready_list);
    }

    current =
    container_of(node,
                 struct thread,
                 sched_node);

    current->state = THREAD_RUNNING;

    return current;
}

/*
 * Switch from the current thread to whichever thread
 * scheduler_next() selects. Shared by scheduler_tick() and
 * scheduler_yield(). Resets the quantum unconditionally,
 * regardless of whether a switch actually occurs.
 */
static void scheduler_switch(void)
{
    ASSERT(!interrupt_enabled());
    struct thread *old;
    struct thread *next;

    old = current;

    next = scheduler_next();

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;

    if (next == old)
        return;

    context_switch(&old->kernel_sp, next->kernel_sp);
}

void scheduler_start(void)
{
    memset(&idle_thread, 0, sizeof(idle_thread));

    list_node_init(&idle_thread.sched_node);

    idle_thread.tid = 0;
    idle_thread.state = THREAD_RUNNING;

    scheduler_add(&idle_thread);

    current = &idle_thread;

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;
}

void scheduler_tick(void)
{
    /* Scheduling has not started yet. */
    if (current == NULL)
        return;

    if (--quantum_remaining > 0)
        return;

    scheduler_switch();
}

void scheduler_yield(void)
{
    ASSERT(!interrupt_enabled());
    if (current == NULL)
        return;

    scheduler_switch();

}
