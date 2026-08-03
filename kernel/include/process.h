#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "list.h"

struct thread;

struct process
{
    pid_t pid;

    /* Linked list of all processes */
    struct list_node process_node;

    /* Threads owned by this process */
    struct list threads;
};


struct process *process_create(void);

#endif
