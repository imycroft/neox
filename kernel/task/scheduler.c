#include "scheduler.h"
#include "scheduler_internal.h"
#include "arch.h"
#include "assert.h"
#include "list.h"
#include "thread.h"
#include "string.h"

#include "util.h"

#include "arch/x86/context.h"
#include "wait.h"
#include "heap.h"

static struct list ready_list;
static struct thread *current;

/*
 * Ticks remaining before the current thread is preempted.
 */
static uint32_t quantum_remaining;

/*
 * The idle thread.  It is never inserted into ready_list;
 * scheduler_next() returns it as a fallback when ready_list
 * is empty.  It owns a real kernel stack so context_switch()
 * can safely switch to it.
 */
static struct thread idle_thread;

/*
 * Idle loop: re-enable interrupts and halt.  The x86 sti+hlt
 * pairing guarantees the CPU sleeps until the next interrupt
 * arrives, at which point scheduler_tick() will preempt into
 * a real thread if one has become ready.
 */
static void scheduler_idle_loop(void)
{
    while (true)
        interrupt_enable_and_halt();
}

void scheduler_init(void)
{
    list_init(&ready_list);

    current = NULL;

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;
}

void scheduler_add(struct thread *thread)
{
    ASSERT(!interrupt_enabled());

    if (thread == NULL)
        return;

    list_push_back(&ready_list,
                   &thread->sched_node);
}

void scheduler_remove(struct thread *thread)
{
    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());

    /*
     * idle_thread is never in ready_list; its sched_node
     * stays NULL/NULL after list_node_init().  Removing it
     * would hit list_remove()'s ASSERT, so skip it here.
     */
    if (thread->sched_node.prev == NULL &&
        thread->sched_node.next == NULL)
        return;

    list_remove(&thread->sched_node);
}

struct thread *scheduler_current(void)
{
    return current;
}

struct thread *scheduler_next(void)
{
    struct list_node *node;

    /*
     * Ready list is empty: fall back to the idle thread.
     * idle_thread always has a valid kernel stack, so
     * context_switch() is safe.
     */
    if (list_empty(&ready_list))
    {
        current        = &idle_thread;
        current->state = THREAD_RUNNING;
        return current;
    }

    /* Bootstrap: no thread has run yet, pick the front. */
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

    /*
     * current's sched_node is NULL/NULL when:
     *   (a) it was just removed because it blocked/terminated, or
     *   (b) current == &idle_thread (never in the list).
     * In both cases jump to the front of the ready list.
     */
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
 * scheduler_next() selects.  Shared by scheduler_tick() and
 * scheduler_yield().  Resets the quantum unconditionally,
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

/*
 * Register the idle thread and make it the current context.
 * Allocates a real kernel stack for idle so that
 * context_switch() can safely switch to it when ready_list
 * is empty.
 *
 * idle_thread is intentionally NOT added to ready_list;
 * scheduler_next() returns it as the fallback directly.
 */
void scheduler_start(void)
{
    interrupt_state_t state;
    uintptr_t *stack;
    struct cpu_context *context;

    state = interrupt_save();

    memset(&idle_thread, 0, sizeof(idle_thread));
    list_node_init(&idle_thread.sched_node);

    idle_thread.tid   = 0;
    idle_thread.state = THREAD_RUNNING;

    /* Allocate a real stack for idle. */
    idle_thread.kernel_stack = kmalloc(THREAD_STACK_SIZE);

    ASSERT(idle_thread.kernel_stack != NULL);

    stack = (uintptr_t *)(
        (uint8_t *)idle_thread.kernel_stack + THREAD_STACK_SIZE
    );

    /* Set up the initial stack identically to thread_create():
     *   [top]   return address  (unreachable — loop never returns)
     *   [top-4] first IP        (scheduler_idle_loop)
     *   [top-8 .. top-20] zeroed cpu_context
     */
    *--stack = (uintptr_t)scheduler_idle_loop; /* safety ret */
    *--stack = (uintptr_t)scheduler_idle_loop; /* first IP   */

    context = (struct cpu_context *)(stack - 4);
    memset(context, 0, sizeof(*context));

    idle_thread.kernel_sp = (uintptr_t)context;

    current           = &idle_thread;
    quantum_remaining = SCHEDULER_QUANTUM_TICKS;

    interrupt_restore(state);
}

void scheduler_restore(struct thread *thread)
{
    ASSERT(!interrupt_enabled());
    ASSERT(thread != NULL);
    
    list_init(&ready_list);
    current = thread;
    current->state = THREAD_RUNNING;
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

void scheduler_terminate(struct thread *thread)
{
    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());

    scheduler_remove(thread);

    thread->state = THREAD_TERMINATED;

    wait_queue_wake_all(&thread->termination_queue);
}

bool scheduler_idle(void)
{
    return current == &idle_thread;
}

uint32_t scheduler_get_quantum_remaining(void)
{
    return quantum_remaining;
}
