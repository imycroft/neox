#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "list.h"

#define PROCESS_NAME_MAX 32

struct page_directory;

struct thread;

struct user_stack
{
    struct list_node node;
    /*
     * Base virtual address of the stack
     */
    uintptr_t virt;
};

struct process
{
    pid_t pid;

    char name[PROCESS_NAME_MAX];

    /* Address space owned by this process */
    struct page_directory *page_directory;

    /* Linked list of all processes */
    struct list_node process_node;

    /*
     * Ring-3 stacks currently allocated in this process.
     *
     * Each entry represents one virtual stack address.
     */
    struct list user_stacks;

    /* Threads owned by this process */
    struct list threads;
};

void process_add(struct process *process);

void process_remove(struct process *process);

struct process *process_find(pid_t pid);

struct process *process_find_by_name(const char *name);

struct process *process_create(const char *name);

/*
 * process_destroy() — remove the process from the global list
 * and free it.  Must only be called after all threads belonging
 * to the process have been destroyed.  Interrupts must be
 * disabled by the caller.
 */
void process_destroy(struct process *process);

#endif
