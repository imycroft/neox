#pragma once

/*
 * Internal scheduler interface.
 *
 * This header is intended only for scheduler-related kernel
 * subsystems (thread, wait queue, etc.). It must not be
 * included by arbitrary kernel code.
 */

void scheduler_yield(void);

void scheduler_terminate(struct thread *thread);

/*
 * Push a terminated detached thread onto the zombie list and
 * wake the reaper.  Called by scheduler_terminate() and by
 * thread_detach() when the thread already terminated.
 * Interrupts must be disabled by the caller.
 */
void scheduler_push_zombie(struct thread *thread);
