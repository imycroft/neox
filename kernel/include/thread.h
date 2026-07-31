#ifndef THREAD_H
#define THREAD_H

#include "memory.h"
#include "types.h"

#define THREAD_STACK_SIZE PAGE_SIZE

struct process;

struct thread
{
    uint32_t tid;

    /* Top of the kernel stack */
    uintptr_t esp;

    /* Base of the allocated kernel stack */
    void *kernel_stack;

    /* Owning process */
    struct process *process;

    /* Linked list */
    struct thread *next;
};

struct thread *thread_create(struct process *process);

#endif
