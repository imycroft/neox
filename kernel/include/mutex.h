#ifndef MUTEX_H
#define MUTEX_H

#include "wait.h"

struct thread;

struct mutex
{
    struct thread *owner;
    struct wait_queue wait_queue;
};

void mutex_init(struct mutex *mutex);

void mutex_lock(struct mutex *mutex);

void mutex_unlock(struct mutex *mutex);

#endif
