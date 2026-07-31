#include "scheduler.h"

#include "thread.h"

static struct thread *ready_list;
static struct thread *current;

void scheduler_init(void)
{
    ready_list = NULL;
    current = NULL;
}

void scheduler_add(struct thread *thread)
{
    struct thread *last;

    if (thread == NULL)
        return;

    thread->next = NULL;

    if (ready_list == NULL)
    {
        ready_list = thread;
        return;
    }

    last = ready_list;

    while (last->next != NULL)
        last = last->next;

    last->next = thread;
}

struct thread *scheduler_current(void)
{
    return current;
}

struct thread *scheduler_next(void)
{
    if (ready_list == NULL)
        return NULL;

    if (current == NULL)
    {
        current = ready_list;
        return current;
    }

    if (current->next != NULL)
    {
        current = current->next;
        return current;
    }

    current = ready_list;
    return current;
}
