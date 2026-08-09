#include "thread.h"
#include "assert.h"
#include "panic.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "process.h"

#include "arch/x86/context.h"
#include "arch.h"

#include "heap.h"
#include "string.h"
#include "wait.h"

// Private functions
static void thread_exit(void)
{
    struct thread *thread;

    interrupt_disable();

    thread = scheduler_current();

    ASSERT(thread != NULL);

    scheduler_terminate(thread);

    scheduler_yield();

    panic("terminated thread resumed");
}

/*
 * Initial entry point for every kernel thread.
 *
 * A thread's first run is reached via context_switch()'s
 * `ret`, not `iret`. When the switch was triggered from
 * inside the timer interrupt handler, interrupts are still
 * disabled at this point (the ISR's `cli` was never undone
 * by a matching `iret`), so it must be re-enabled explicitly
 * here for preemption of this thread to begin.
 */
static void thread_bootstrap(void)
{
    struct thread *thread;

    interrupt_enable();

    thread = scheduler_current();

    thread->entry();

    thread_exit();
}

static uint32_t next_tid = 1;

// API Functions

void thread_add(struct thread *thread)
{
    ASSERT(thread != NULL);

    list_push_back(&thread->process->threads,
                   &thread->group_node);

    scheduler_add(thread);
}
void thread_block(void)
{
    interrupt_state_t state;

    ASSERT(scheduler_current() != NULL);

    state = interrupt_save();

    struct thread *thread = scheduler_current();

    thread->state = THREAD_BLOCKED;
    scheduler_remove(thread);
    scheduler_yield();

    interrupt_restore(state);
}

void thread_unblock(struct thread *thread)
{
    interrupt_state_t state;

    ASSERT(thread != NULL);
    ASSERT(thread->state == THREAD_BLOCKED);

    state = interrupt_save();

    thread->state = THREAD_READY;

    scheduler_add(thread);

    interrupt_restore(state);
}

struct thread *thread_create(
    struct process *process,
    void (*entry)(void)
)
{
    struct thread *thread;

    ASSERT(process != NULL);

    thread = kmalloc(sizeof(*thread));

    if (thread == NULL)
        return NULL;

    memset(thread, 0, sizeof(*thread));

    list_node_init(&thread->group_node);
    list_node_init(&thread->sched_node);
    list_node_init(&thread->wait_node);

    wait_queue_init(&thread->termination_queue);

    thread->wait_queue = NULL;

    thread->entry = entry;

    thread->kernel_stack = kmalloc(THREAD_STACK_SIZE);

    if (thread->kernel_stack == NULL)
    {
        kfree(thread);
        return NULL;
    }
    uintptr_t *stack;
    struct cpu_context *context;

    stack =
    (uintptr_t *)
    (
        (uint8_t *)thread->kernel_stack +
        THREAD_STACK_SIZE
    );

    /* Return address if the thread entry function exits. */
    *--stack = (uintptr_t)thread_exit;

    /* Initial instruction pointer. */
    *--stack = (uintptr_t)thread_bootstrap;

    /* Reserve space for saved registers. */
    context = (struct cpu_context *)(stack - 4);

    memset(context, 0, sizeof(*context));

    thread->kernel_sp = (uintptr_t)context;

    thread->state = THREAD_READY;

    thread->tid = next_tid++;
    thread->process = process;

    return thread;
}

void thread_wait(struct thread *thread)
{
    interrupt_state_t state;

    ASSERT(thread != NULL);

    state = interrupt_save();

    if (thread->state != THREAD_TERMINATED)
        wait_queue_sleep(&thread->termination_queue);

    interrupt_restore(state);
}

void thread_yield(void)
{
    if (scheduler_current() == NULL)
        return;

    interrupt_state_t state;

    state = interrupt_save();

    scheduler_yield();

    interrupt_restore(state);
}
