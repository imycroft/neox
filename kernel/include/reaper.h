#ifndef REAPER_H
#define REAPER_H

#include "wait.h"
#include "thread.h"

/*
 * Start the reaper kernel thread.
 * Must be called before any detached thread can terminate.
 */
void reaper_init(void);

/*
 * Return the wait queue the reaper sleeps on, or NULL before
 * reaper_init() has been called.  Used by scheduler_push_zombie()
 * to wake the reaper.
 */
struct wait_queue *reaper_wait_queue(void);

/*
 * Return the reaper thread pointer.
 * Used by scheduler_terminate() to ASSERT the reaper never
 * accidentally destroys itself.
 */
struct thread *reaper_thread_get(void);

#endif
