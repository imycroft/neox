#include "process.h"

#include "heap.h"
#include "string.h"

static uint32_t next_pid = 1;

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

    return process;
}
