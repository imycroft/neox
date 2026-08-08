#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "types.h"
#include "wait.h"

struct semaphore
{
    uint32_t count;
    struct wait_queue wait_queue;
};

void semaphore_init(
    struct semaphore *sem,
    uint32_t count
);

void semaphore_acquire(
    struct semaphore *sem
);

void semaphore_release(
    struct semaphore *sem
);

#endif
