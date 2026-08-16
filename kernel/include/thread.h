#ifndef THREAD_H
#define THREAD_H

#include "memory.h"
#include "types.h"

#include "list.h"
#include "wait.h"

#define THREAD_STACK_SIZE PAGE_SIZE

#define THREAD_STACK_PAGES \
((THREAD_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)

enum thread_state
{
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
};

struct process;

struct thread
{
    tid_t tid;

    /* Top of the kernel stack */
    uintptr_t kernel_sp;

    /* Base virtual address of the kernel stack. */
    void *kernel_stack;

    /* Thread entry point */
    void (*entry)(void);

    /* Current execution state */
    enum thread_state state;

    /* Owning process */
    struct process *process;

    struct list_node group_node;
    struct list_node sched_node;
    struct list_node wait_node;

    /*
     * Links this thread into the scheduler's zombie_list after
     * termination.  NULL/NULL when the thread is not a zombie.
     */
    struct list_node zombie_node;

    /*
     * When true the reaper owns this thread's lifetime and will
     * call thread_destroy() once it terminates.
     * When false the caller is expected to call thread_join().
     */
    bool detached;

    /*
     * Threads waiting for this thread to terminate sleep here.
     */
    struct wait_queue termination_queue;

    /*
     * Wait queue this thread is currently sleeping in.
     */
    struct wait_queue *wait_queue;

    /*
     * Non-NULL when this thread is a user-mode thread.
     * Points to a `struct usermode_desc` (defined in usermode.c)
     * that holds the Ring-3 entry point and user stack info.
     * The trampoline frees it after jumping to Ring 3.
     */
    void *usermode_desc;
};

void thread_add(struct thread *thread);

struct thread *thread_create(
    struct process *process,
    void (*entry)(void)
);

void thread_block(void);

void thread_unblock(struct thread *thread);

/*
 * thread_join() — wait for a joinable (non-detached) thread to
 * terminate, then destroy it.  The pointer is invalid after this
 * returns; the caller must not dereference it.
 */
void thread_join(struct thread *thread);

/*
 * thread_detach() — transfer lifetime ownership to the reaper.
 * The caller must not touch the pointer again after this returns.
 * If the thread has already terminated the reaper is signalled
 * immediately.
 */
void thread_detach(struct thread *thread);

/*
 * thread_destroy() — free the kernel stack and the struct itself.
 * Must only be called by thread_join() or the reaper, never by
 * the thread itself.  Interrupts must be disabled by the caller.
 */
void thread_destroy(struct thread *thread);

void thread_yield(void);




#endif
