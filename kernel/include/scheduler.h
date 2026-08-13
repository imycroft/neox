#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "thread.h"

/*
 * Number of timer ticks a thread is allowed to run before
 * being preempted in favor of the next ready thread.
 */
#define SCHEDULER_QUANTUM_TICKS 5
// DEBUG
uint32_t scheduler_get_quantum_remaining(void);
// END DEBUG
void scheduler_init(void);

void scheduler_add(struct thread *thread);

void scheduler_remove(struct thread *thread);

struct thread *scheduler_current(void);

struct thread *scheduler_next(void);

/*
 * Begin scheduling.
 *
 * The calling context (the boot stack) becomes the idle
 * thread and is registered with the scheduler. Must be
 * called once, after scheduler_init(), before any timer
 * tick can be allowed to preempt.
 */
void scheduler_start(void);

// for tests
void scheduler_restore(struct thread *thread);

/*
 * Timer tick notification.
 *
 * Called from the timer interrupt handler on every PIT
 * tick. Decrements the current thread's remaining quantum
 * and preempts it once the quantum is exhausted.
 *
 * Safe to call before scheduler_start(); does nothing until
 * scheduling has begun.
 */
void scheduler_tick(void);

bool scheduler_idle(void);

/*
 * Remove and return the next terminated thread from the zombie
 * list, or NULL if the list is empty.
 * Interrupts must be disabled by the caller.
 */
struct thread *scheduler_next_zombie(void);
void scheduler_enter_idle(uintptr_t idle_sp);
uintptr_t scheduler_idle_stack_pointer(void);


#endif
