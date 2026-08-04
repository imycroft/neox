#ifndef WAIT_H
#define WAIT_H

#include "list.h"

struct thread;

struct wait_queue
{
    struct list threads;
};

void wait_queue_init(struct wait_queue *queue);

void wait_queue_add(
    struct wait_queue *queue,
    struct thread *thread
);

struct thread *wait_queue_remove(
    struct wait_queue *queue
);

void wait_queue_wake(struct wait_queue *queue);
void wait_queue_wake_all(struct wait_queue *queue);

#endif
