#include "process.h"

#include "heap.h"
#include "string.h"
#include "util.h"
#include "assert.h"
#include "arch.h"
#include "list.h"
#include "paging.h"

#include "printf.h"

static uint32_t next_pid = 1;

static struct list process_list =
{
    .head =
    {
        .prev = &process_list.head,
        .next = &process_list.head
    }
};

void process_add(struct process *process)
{
    ASSERT(process != NULL);

    list_push_back(&process_list,
                   &process->process_node);
}

void process_remove(struct process *process)
{
    ASSERT(process != NULL);

    list_remove(&process->process_node);
}

struct process *process_find(pid_t pid)
{
    struct list_node *node;
    struct process *process;

    for (node = list_front(&process_list);
         node != &process_list.head;
    node = list_next(node))
         {
             process = container_of(node,
                                    struct process,
                                    process_node);

             if (process->pid == pid)
                 return process;
         }

         return NULL;
}

struct process *process_find_by_name(
    const char *name
)
{
    struct list_node *node;
    struct process *process;

    if (name == NULL)
        return NULL;

    for (node = list_front(&process_list);
         node != &process_list.head;
    node = list_next(node))
         {
             process = container_of(node,
                                    struct process,
                                    process_node);

             if (strcmp(process->name, name) == 0)
                 return process;
         }

         return NULL;
}

struct process *process_create(const char *name)
{
    struct process *process;

    process = kmalloc(sizeof(*process));

    if (process == NULL)
        return NULL;

    memset(process, 0, sizeof(*process));

    list_node_init(&process->process_node);
    list_init(&process->threads);
    list_init(&process->user_stacks);

    process->page_directory = paging_create_directory();

    if (process->page_directory == NULL)
    {
        kfree(process);
        return NULL;
    }

    process->pid = next_pid++;

    strncpy(process->name,
            name,
            PROCESS_NAME_MAX - 1);

    process->name[PROCESS_NAME_MAX - 1] = '\0';

    process_add(process);

    return process;
}

void process_destroy(struct process *process)
{
    ASSERT(process != NULL);
    ASSERT(!interrupt_enabled());
    ASSERT(list_empty(&process->threads));

    /*
     * Every user-mode thread must have released its user stack
     * before the process can be destroyed.
     */
    ASSERT(list_empty(&process->user_stacks));

    process_remove(process);

    if (process->page_directory != NULL)
        paging_destroy_directory(process->page_directory);

    kfree(process);
}
