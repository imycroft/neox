#ifndef CONDVAR_H
#define CONDVAR_H

#include "wait.h"

struct mutex;

/*
 * Condition variable.
 *
 * A condvar must always be used with the same mutex.  The caller
 * holds the mutex when calling condvar_wait(); the mutex is
 * released and the thread enqueued atomically so that no signal
 * can be lost between the two operations (the classic lost-wakeup
 * bug).  The mutex is re-acquired before condvar_wait() returns.
 *
 * condvar_signal() wakes one waiter.
 * condvar_broadcast() wakes all waiters.
 *
 * Neither signal nor broadcast requires the mutex to be held by
 * the caller, but holding it is the conventional and safer usage.
 */

struct condvar
{
    struct wait_queue wait_queue;
};

void condvar_init(struct condvar *cv);

/*
 * Atomically release the mutex and sleep on the condvar.
 * Re-acquires the mutex before returning.
 *
 * Precondition : caller holds mutex.
 * Postcondition: caller holds mutex.
 */
void condvar_wait(struct condvar *cv, struct mutex *mutex);

/*
 * Wake one thread waiting on cv (if any).
 */
void condvar_signal(struct condvar *cv);

/*
 * Wake all threads waiting on cv.
 */
void condvar_broadcast(struct condvar *cv);

#endif
