#include "thread.h"
#include "scheduler.h"

#include "arch/x86/context.h"

#include "heap.h"
#include "string.h"

#include "printf.h"
// Private functions
static void thread_exit(void)
{
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

/*
 * Initial entry point for every kernel thread.
 */
static void thread_bootstrap(void)
{

    struct thread *thread;

    printf("Entered thread_bootstrap\n");

    thread = scheduler_current();

    printf("thread = %x\n", (uint32_t)thread);

    thread->entry();

    thread_exit();
}

static uint32_t next_tid = 1;

// API Functions

struct thread *thread_create(
    struct process *process,
    void (*entry)(void)
)
{
    struct thread *thread;

    thread = kmalloc(sizeof(*thread));

    if (thread == NULL)
        return NULL;

    memset(thread, 0, sizeof(*thread));

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


