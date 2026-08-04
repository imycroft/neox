#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "list.h"

#define PROCESS_NAME_MAX 32

struct thread;

struct process
{
    pid_t pid;

    char name[PROCESS_NAME_MAX];

    /* Linked list of all processes */
    struct list_node process_node;

    /* Threads owned by this process */
    struct list threads;
};

void process_add(struct process *process);

void process_remove(struct process *process);

struct process *process_find(pid_t pid);

struct process *process_create(const char *name);

#endif
