#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

struct thread;

struct process
{
    uint32_t pid;

    /* First thread owned by this process */
    struct thread *threads;

    /* Linked list of all processes */
    struct process *next;
};

struct process *process_create(void);

#endif
