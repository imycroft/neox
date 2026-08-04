#include "thread.h"
#include "assert.h"
#include "scheduler.h"
#include "process.h"

#include "arch/x86/context.h"

#include "heap.h"
#include "string.h"
#include "wait.h"

// Private functions
static void thread_exit(void)
{
    struct thread *thread;

    thread = scheduler_current();

    if (thread != NULL)
        thread->state = THREAD_TERMINATED;

    /*
     * Reclaiming the thread's process/stack/tid is future work.
     * The thread remains in the ready list and continues to be
     * scheduled, but simply halts until preempted again.
     */
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
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

    __asm__ volatile ("sti");

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
    struct thread *thread;

    thread = scheduler_current();

    ASSERT(thread != NULL);

    __asm__ volatile ("cli");

    thread->state = THREAD_BLOCKED;

    scheduler_remove(thread);

    scheduler_yield();
}

void thread_unblock(struct thread *thread)
{
    ASSERT(thread != NULL);

    ASSERT(thread->state == THREAD_BLOCKED);

    thread->state = THREAD_READY;

    scheduler_add(thread);
}

void thread_wait(struct wait_queue *queue)
{
    struct thread *thread;

    ASSERT(queue != NULL);

    thread = scheduler_current();

    ASSERT(thread != NULL);

    wait_queue_add(queue, thread);

    thread_block();
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



