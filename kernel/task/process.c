#include "process.h"

#include "heap.h"
#include "string.h"
#include "util.h"

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
    if (process == NULL)
        return;

    list_push_back(&process_list,
                   &process->process_node);
}

void process_remove(struct process *process)
{
    if (process == NULL)
        return;

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

struct process *process_create(const char *name)
{
    struct process *process;

    process = kmalloc(sizeof(*process));

    if (process == NULL)
        return NULL;

    memset(process, 0, sizeof(*process));

    list_node_init(&process->process_node);

    list_init(&process->threads);

    process->pid = next_pid++;

    strncpy(process->name,
            name,
            PROCESS_NAME_MAX - 1);

    process->name[PROCESS_NAME_MAX - 1] = '\0';

    process_add(process);

    return process;
}
