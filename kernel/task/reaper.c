#include "reaper.h"

#include "arch.h"
#include "assert.h"
#include "process.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "thread.h"
#include "wait.h"

#include "printf.h"

static struct thread    *reaper_thread;
static struct wait_queue reaper_wq;
static bool              reaper_ready = false;

struct wait_queue *reaper_wait_queue(void)
{
    return reaper_ready ? &reaper_wq : NULL;
}

struct thread *reaper_thread_get(void)
{
    return reaper_thread;
}

static void reaper_entry(void)
{
    struct thread    *zombie;
    struct process   *process;
    interrupt_state_t state;

    while (true)
    {
        /*
         * Sleep until a detached thread terminates.
         * wait_queue_sleep() requires interrupts to be disabled;
         * it re-enables them internally when it yields and
         * restores the saved state on wake.
         */
        state = interrupt_save();
        wait_queue_sleep(&reaper_wq);
        interrupt_restore(state);

        /*
         * Drain every zombie that accumulated while we slept.
         * Keep interrupts disabled for the entire destroy
         * sequence: kfree() is not ISR-reentrant and we must
         * not let a tick slip in between popping the zombie and
         * freeing its stack.
         */
        state = interrupt_save();


        while ((zombie = scheduler_next_zombie()) != NULL)
        {
            printf("zombie on destruction = %s\n", zombie->process->name);
            /* Reaper must never reap itself. */
            ASSERT(zombie != reaper_thread);

            process = zombie->process;

            /*
             * thread_destroy() removes group_node from
             * process->threads, frees the kernel stack, and
             * frees the struct.  zombie is invalid after this.
             */
            thread_destroy(zombie);

            if (process != NULL && list_empty(&process->threads))
                process_destroy(process);
        }

        interrupt_restore(state);
    }
}

void reaper_init(void)
{
    struct process    *process;
    interrupt_state_t  state;

    wait_queue_init(&reaper_wq);
    reaper_ready = true;

    process = process_create("reaper");

    ASSERT(process != NULL);

    reaper_thread = thread_create(process, reaper_entry);

    ASSERT(reaper_thread != NULL);

    /*
     * Mark the reaper detached so its lifecycle flag is
     * consistent: it never terminates under normal operation,
     * but if it ever did, the ASSERT in scheduler_terminate()
     * would fire before any detached-path logic ran.
     */
    reaper_thread->detached = true;

    state = interrupt_save();
    thread_add(reaper_thread);
    interrupt_restore(state);
}
