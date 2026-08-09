#include "mutex.h"
#include "arch.h"
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

    interrupt_state_t state = interrupt_save();

    if (mutex->owner == NULL)
    {
        mutex->owner = scheduler_current();
        interrupt_restore(state);
        return;
    }

    wait_queue_sleep(&mutex->wait_queue);

    interrupt_restore(state);
}

void mutex_unlock(struct mutex *mutex)
{
    ASSERT(mutex != NULL);
    ASSERT(mutex->owner == scheduler_current());

    interrupt_state_t state = interrupt_save();

    struct thread *thread = wait_queue_remove(&mutex->wait_queue);

    if (thread != NULL)
    {
        mutex->owner = thread;
        interrupt_restore(state);      // restore before unblock (unblock saves its own)
        thread_unblock(thread);
        return;
    }

    mutex->owner = NULL;

    interrupt_restore(state);
}
