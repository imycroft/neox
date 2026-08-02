#include "scheduler.h"

#include "thread.h"
#include "string.h"

#include "arch/x86/context.h"

static struct thread *ready_list;
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
    ready_list = NULL;
    current = NULL;
    quantum_remaining = SCHEDULER_QUANTUM_TICKS;
}

void scheduler_add(struct thread *thread)
{
    struct thread *last;

    if (thread == NULL)
        return;

    thread->next = NULL;

    if (ready_list == NULL)
    {
        ready_list = thread;
        return;
    }

    last = ready_list;

    while (last->next != NULL)
        last = last->next;

    last->next = thread;
}

struct thread *scheduler_current(void)
{
    return current;
}

struct thread *scheduler_next(void)
{
    if (ready_list == NULL)
        return NULL;

    if (current == NULL)
    {
        current = ready_list;
        current->state = THREAD_RUNNING;
        return current;
    }

    current->state = THREAD_READY;

    if (current->next != NULL)
    {
        current = current->next;
    }
    else
    {
        current = ready_list;
    }

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
    if (current == NULL)
        return;

    scheduler_switch();
}
