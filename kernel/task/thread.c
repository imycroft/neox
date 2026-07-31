#include "thread.h"

#include "heap.h"
#include "string.h"


static uint32_t next_tid = 1;

struct thread *thread_create(struct process *process)
{
    struct thread *thread;

    thread = kmalloc(sizeof(*thread));

    if (thread == NULL)
        return NULL;

    memset(thread, 0, sizeof(*thread));

    thread->kernel_stack = kmalloc(THREAD_STACK_SIZE);

    if (thread->kernel_stack == NULL)
    {
        kfree(thread);
        return NULL;
    }

    thread->esp =
    (uintptr_t)thread->kernel_stack +
    THREAD_STACK_SIZE;

    thread->tid = next_tid++;
    thread->process = process;

    return thread;
}
