#include "process.h"

#include "heap.h"
#include "string.h"

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

struct process *process_create(void)
{
    struct process *process;

    process = kmalloc(sizeof(*process));

    if (process == NULL)
        return NULL;

    memset(process, 0, sizeof(*process));

    list_node_init(&process->process_node);

    list_init(&process->threads);

    process->pid = next_pid++;

    process_add(process);

    return process;
}
