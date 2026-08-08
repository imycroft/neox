#include "semaphore.h"
#include "arch.h"
#include "assert.h"
#include "thread.h"

void semaphore_init(struct semaphore *sem,
                    uint32_t count)
{
    ASSERT(sem != NULL);

    sem->count = count;

    wait_queue_init(&sem->wait_queue);
}

void semaphore_acquire(struct semaphore *sem)
{
    ASSERT(sem != NULL);

    if (sem->count > 0)
    {
        sem->count--;
        return;
    }

    interrupt_state_t state;
    state = interrupt_save();

    wait_queue_sleep(&sem->wait_queue);

    interrupt_restore(state);
}

void semaphore_release(struct semaphore *sem)
{
    struct thread *thread;

    ASSERT(sem != NULL);

    thread = wait_queue_remove(&sem->wait_queue);

    if (thread != NULL)
    {
        thread_unblock(thread);
        return;
    }

    sem->count++;
}
