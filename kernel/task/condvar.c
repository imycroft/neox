#include "condvar.h"
#include "mutex.h"
#include "wait.h"
#include "arch.h"
#include "assert.h"

void condvar_init(struct condvar *cv)
{
    ASSERT(cv != NULL);

    wait_queue_init(&cv->wait_queue);
}

/*
 * condvar_wait — atomic release-and-sleep.
 *
 * The lost-wakeup problem:
 *
 *   Without atomicity, a signal could arrive after the mutex is
 *   released but before the thread is enqueued on the condvar.
 *   The wakeup would find an empty queue and be silently dropped.
 *   The waiting thread then sleeps forever, never knowing the
 *   condition it was waiting for already became true.
 *
 * The fix:
 *
 *   Disable interrupts before releasing the mutex.  With
 *   interrupts off, no other thread can run between
 *   mutex_unlock() and wait_queue_sleep() — the enqueue and
 *   the release are a single atomic unit from every other
 *   thread's perspective.
 *
 *   wait_queue_sleep() itself asserts that interrupts are off,
 *   which enforces the discipline on every caller.
 *
 * Sequence:
 *
 *   1. interrupt_save()      — cli, capture IF
 *   2. mutex_unlock(mutex)   — release; signallers can now
 *                              acquire the mutex but cannot
 *                              run yet (interrupts off)
 *   3. wait_queue_sleep()    — enqueue + block + yield
 *                              (interrupts re-enabled when the
 *                              next thread runs via thread_bootstrap
 *                              or interrupt_restore in thread_block)
 *   4. [thread is woken by condvar_signal / condvar_broadcast]
 *   5. mutex_lock(mutex)     — re-acquire before returning
 *   6. interrupt_restore()   — restore IF to original state
 */
void condvar_wait(struct condvar *cv, struct mutex *mutex)
{
    interrupt_state_t state;

    ASSERT(cv    != NULL);
    ASSERT(mutex != NULL);

    state = interrupt_save();

    /*
     * Release the mutex while interrupts are off.  Any thread
     * that calls condvar_signal() will acquire the mutex first,
     * but it cannot actually run until we yield inside
     * wait_queue_sleep(), so the enqueue below is guaranteed to
     * happen before any signal is processed.
     */
    mutex_unlock(mutex);

    /*
     * Enqueue on the condvar and yield.  Interrupts are off;
     * wait_queue_sleep() asserts this.  We sleep until
     * condvar_signal() or condvar_broadcast() calls
     * thread_unblock() on us.
     */
    wait_queue_sleep(&cv->wait_queue);

    /*
     * We have been woken.  Re-acquire the mutex before returning
     * to the caller, who expects to hold it again.
     *
     * mutex_lock() may block if another thread raced to acquire
     * the mutex between our wakeup and this call.  That is
     * correct behaviour — the condvar contract only guarantees
     * the condition *may* be true, not that the mutex is free.
     *
     * Restore interrupts first so that mutex_lock()'s slow path
     * (wait_queue_sleep) can operate with the correct IF state.
     */
    interrupt_restore(state);

    mutex_lock(mutex);
}

/*
 * condvar_signal — wake one waiter.
 *
 * The woken thread will compete to re-acquire the mutex when it
 * resumes.  It is not given the mutex directly (unlike the mutex
 * ownership-transfer model) because the signaller may still hold
 * the mutex and intends to release it after the signal.
 */
void condvar_signal(struct condvar *cv)
{
    ASSERT(cv != NULL);

    interrupt_state_t state = interrupt_save();

    wait_queue_wake(&cv->wait_queue);

    interrupt_restore(state);
}

/*
 * condvar_broadcast — wake all waiters.
 *
 * All sleeping threads are made runnable.  Each will then
 * compete to re-acquire the mutex individually.  Only one will
 * succeed at a time — the rest will block in mutex_lock() until
 * the mutex is released again.
 */
void condvar_broadcast(struct condvar *cv)
{
    ASSERT(cv != NULL);

    interrupt_state_t state = interrupt_save();

    wait_queue_wake_all(&cv->wait_queue);

    interrupt_restore(state);
}
