#ifndef THREAD_H
#define THREAD_H

#include "memory.h"
#include "types.h"

#define THREAD_STACK_SIZE PAGE_SIZE

enum thread_state
{
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
};

struct process;

struct thread
{
    uint32_t tid;

    /* Top of the kernel stack */
    uintptr_t kernel_sp;

    /* Base of the allocated kernel stack */
    void *kernel_stack;

    /* Thread entry point */
    void (*entry)(void);

    /* Current execution state */
    enum thread_state state;

    /* Owning process */
    struct process *process;

    /* Linked list */
    struct thread *next;
};

struct thread *thread_create(
    struct process *process,
    void (*entry)(void)
);

#endif
