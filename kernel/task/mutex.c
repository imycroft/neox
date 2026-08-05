#include "mutex.h"
#include "assert.h"
#include "scheduler.h"
#include "types.h"

void mutex_init(struct mutex *mutex)
{
    ASSERT(mutex != NULL);

    mutex->owner = NULL;

    wait_queue_init(&mutex->wait_queue);
}

void mutex_lock(struct mutex *mutex)
{
    ASSERT(mutex != NULL);

    if (mutex->owner == NULL)
    {
        mutex->owner = scheduler_current();
        return;
    }

    wait_queue_sleep(&mutex->wait_queue);
}

void mutex_unlock(struct mutex *mutex)
{
    struct thread *thread;

    ASSERT(mutex != NULL);
    ASSERT(mutex->owner == scheduler_current());

    thread = wait_queue_remove(&mutex->wait_queue);

    if (thread != NULL)
    {
        mutex->owner = thread;

        thread_unblock(thread);

        return;
    }

    mutex->owner = NULL;
}
