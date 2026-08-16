#include "scheduler.h"
#include "scheduler_internal.h"
#include "arch.h"
#include "assert.h"
#include "list.h"
#include "thread.h"
#include "vam.h"
#include "pmm.h"
#include "string.h"
#include "tss.h"

#include "util.h"

#include "arch/x86/context.h"
#include "wait.h"
#include "heap.h"
#include "reaper.h"
#include "process.h"
#include "paging.h"
#include "printf.h"
static struct list ready_list;
static struct thread *current;

/*
 * Terminated detached threads whose stacks cannot be freed by
 * themselves are placed here for the reaper thread to drain.
 */
static struct list zombie_list;

/*
 * Ticks remaining before the current thread is preempted.
 */
static uint32_t quantum_remaining;

/*
 * The idle thread.  It is never inserted into ready_list;
 * scheduler_next() returns it as a fallback when ready_list
 * is empty.  It owns a real kernel stack so context_switch()
 * can safely switch to it.
 */
static struct thread idle_thread;

/*
 * Idle loop: re-enable interrupts and halt.  The x86 sti+hlt
 * pairing guarantees the CPU sleeps until the next interrupt
 * arrives, at which point scheduler_tick() will preempt into
 * a real thread if one has become ready.
 */

static void scheduler_idle_loop(void)
{
    while (true)
        interrupt_enable_and_halt();
}
void get_ready_list(void)
{
    return;
}
void scheduler_init(void)
{

    list_init(&ready_list);
    list_init(&zombie_list);

    current = NULL;

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;

}

void scheduler_add(struct thread *thread)
{
    ASSERT(!interrupt_enabled());

    if (thread == NULL)
        return;

    list_push_back(&ready_list,
                   &thread->sched_node);

    get_ready_list();

}

void scheduler_remove(struct thread *thread)
{
    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());

    /*
     * idle_thread is never in ready_list; its sched_node
     * stays NULL/NULL after list_node_init().  Removing it
     * would hit list_remove()'s ASSERT, so skip it here.
     */
    if (thread->sched_node.prev == NULL &&
        thread->sched_node.next == NULL)
        return;

    list_remove(&thread->sched_node);

    get_ready_list();
}

struct thread *scheduler_current(void)
{
    return current;
}

struct thread *scheduler_next(void)
{

    struct list_node *node;

    /*
     * Ready list is empty: fall back to the idle thread.
     * idle_thread always has a valid kernel stack, so
     * context_switch() is safe.
     */

    if (list_empty(&ready_list))
    {
        current        = &idle_thread;
        current->state = THREAD_RUNNING;
        return current;
    }

    /* Bootstrap: no thread has run yet, pick the front. */
    if (current == NULL)
    {
        node = list_front(&ready_list);

        current =
        container_of(node,
                     struct thread,
                     sched_node);

        current->state = THREAD_RUNNING;

        return current;
    }

    /*
     * Only demote a real thread back to READY; idle_thread is
     * never in the ready list and must never be marked READY
     * (it would confuse any code that inspects thread state).
     */
    if (current->state == THREAD_RUNNING && current != &idle_thread)
        current->state = THREAD_READY;


    /*
     * current's sched_node is NULL/NULL when:
     *   (a) it was just removed because it blocked/terminated, or
     *   (b) current == &idle_thread (never in the list).
     * In both cases jump to the front of the ready list.
     */
    if (current->sched_node.next == NULL &&
        current->sched_node.prev == NULL)
    {
        node = list_front(&ready_list);
    }
    else
    {
        node = list_next(&current->sched_node);

        if (node == &ready_list.head)
            node = list_front(&ready_list);
    }

    current =
    container_of(node,
                 struct thread,
                 sched_node);

    current->state = THREAD_RUNNING;

    return current;
}

/*
 * Switch from the current thread to whichever thread
 * scheduler_next() selects.  Shared by scheduler_tick() and
 * scheduler_yield().  Resets the quantum unconditionally,
 * regardless of whether a switch actually occurs.
 */
static void scheduler_switch(void)
{

    ASSERT(!interrupt_enabled());

    struct thread *old;
    struct thread *next;

    old = current;

    next = scheduler_next();

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;

    if (next == old)
        return;

    /*
     * Update the TSS kernel stack pointer before switching.
     * When the next thread is running in Ring 3 and gets interrupted,
     * the CPU reads esp0 from the TSS to find the kernel stack.
     * For pure kernel threads this is a no-op in practice (they never
     * enter Ring 3), but it must always be correct.
     */

    tss_set_kernel_stack(
        (uintptr_t)next->kernel_stack + THREAD_STACK_SIZE
    );

    /*
     * Switch to the address space belonging to the next process.
     *
     * page_directory is a higher-half virtual address, but CR3 must
     * receive the physical address of the page directory.
     *
     * Kernel threads and user threads both belong to a process, so
     * every scheduled thread gets its process address space loaded.
     */

    if (next->process != NULL)
    {
        paging_load_directory(
            VIRT_TO_PHYS(
                (uintptr_t)next->process->page_directory
            )
        );
    }

    context_switch(
        &old->kernel_sp,
        next->kernel_sp
    );

}

/*
 * Register the idle thread and make it the current context.
 * Allocates a real kernel stack for idle so that
 * context_switch() can safely switch to it when ready_list
 * is empty.
 *
 * idle_thread is intentionally NOT added to ready_list;
 * scheduler_next() returns it as the fallback directly.
 */
void scheduler_start(void)
{
    uintptr_t *stack;
    struct cpu_context *context;

    /*
     * The scheduler must remain protected until the bootstrap
     * context has handed control to the idle thread.
     */
    interrupt_disable();

    memset(&idle_thread, 0, sizeof(idle_thread));

    list_node_init(&idle_thread.sched_node);

    idle_thread.tid = 0;
    idle_thread.state = THREAD_RUNNING;

    /*
     * Allocate a virtual stack slot from the kernel stack region.
     */
    idle_thread.kernel_stack = vam_alloc_stack();

    ASSERT(idle_thread.kernel_stack != NULL);

    uint32_t i;
    uint32_t stack_pages;
    void *phys;
    uintptr_t virt;

    stack_pages = KERNEL_STACK_SIZE / PAGE_SIZE;

    for (i = 0; i < stack_pages; i++)
    {
        phys = pmm_alloc_page();

        ASSERT(phys != NULL);

        virt = (uintptr_t)idle_thread.kernel_stack +
        (uintptr_t)i * PAGE_SIZE;

        paging_map_kernel(
            virt,
            (uintptr_t)phys,
                          PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    /*
     * Build the initial stack expected by context_switch()
     * and scheduler_enter_idle().
     *
     * Stack layout:
     *
     *     EDI
     *     ESI
     *     EBX
     *     EBP
     *     return address
     */
    stack = (uintptr_t *)(
        (uint8_t *)idle_thread.kernel_stack + THREAD_STACK_SIZE
    );

    /*
     * RET will enter scheduler_idle_loop().
     */
    *--stack = (uintptr_t)scheduler_idle_loop;

    /*
     * Reserve the four callee-saved registers.
     */
    context = (struct cpu_context *)(stack - 4);

    memset(context, 0, sizeof(*context));

    /*
     * context_switch() expects kernel_sp to point at EDI.
     */
    idle_thread.kernel_sp = (uintptr_t)context;

    /*
     * The scheduler considers idle_thread to be running.
     */
    current = &idle_thread;

    quantum_remaining = SCHEDULER_QUANTUM_TICKS;

    /*
     * IMPORTANT:
     *
     * Interrupts intentionally remain disabled.
     *
     * kernel_main() will create the initial threads and then perform
     * the final bootstrap -> idle handoff.
     */
}

void scheduler_restore(struct thread *thread)
{
    ASSERT(!interrupt_enabled());
    ASSERT(thread != NULL);

    list_init(&ready_list);

    /*
     * Re-initialise the node so list_push_back()'s ASSERT
     * (prev == NULL && next == NULL) passes cleanly, even if
     * the node was previously linked into a list that was
     * wiped by scheduler_init() during a test.
     */

    list_node_init(&thread->sched_node);

    /*
     * Put the init/test thread back into the ready list so
     * that round-robin scheduling can return to it after
     * worker threads consume their quanta.  Without this
     * the thread is invisible to scheduler_next() and
     * test_wait_ticks() deadlocks waiting for ticks that
     * are never delivered to it.
     */

    list_push_back(&ready_list, &thread->sched_node);

    current = thread;
    current->state = THREAD_RUNNING;
    quantum_remaining = SCHEDULER_QUANTUM_TICKS;
}

void scheduler_tick(void)
{
    /* Scheduling has not started yet. */
    if (current == NULL)
        return;

    if (--quantum_remaining > 0)
        return;

    scheduler_switch();
}

void scheduler_yield(void)
{
    ASSERT(!interrupt_enabled());

    if (current == NULL)
        return;

    scheduler_switch();
}

struct thread *scheduler_next_zombie(void)
{
    struct list_node *node;

    ASSERT(!interrupt_enabled());

    if (list_empty(&zombie_list))
        return NULL;

    node = list_front(&zombie_list);
    list_remove(node);

    return container_of(node, struct thread, zombie_node);
}

/*
 * scheduler_push_zombie() — push a terminated detached thread
 * onto the zombie list and wake the reaper.
 *
 * Returns early without pushing if the reaper is not yet
 * initialised (early boot, before reaper_init()).  This is safe
 * because no detached thread can exist before reaper_init() runs.
 *
 * Interrupts must be disabled by the caller.
 */
void scheduler_push_zombie(struct thread *thread)
{
    struct wait_queue *wq;

    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());
    ASSERT(thread->state == THREAD_TERMINATED);

    wq = reaper_wait_queue();

    if (wq == NULL)
        return;

    list_push_back(&zombie_list, &thread->zombie_node);
    wait_queue_wake(wq);
}

void scheduler_terminate(struct thread *thread)
{
    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());

    /*
     * The reaper must never terminate — it would have no one
     * left to free it and would leak itself permanently.
     */
    ASSERT(thread != reaper_thread_get());

    scheduler_remove(thread);

    thread->state = THREAD_TERMINATED;

    /*
     * Wake any thread blocked in thread_join() on this thread.
     */
    wait_queue_wake_all(&thread->termination_queue);

    /*
     * For detached threads nobody calls thread_join(), so hand
     * the struct to the reaper for cleanup.
     */
    if (thread->detached)
        scheduler_push_zombie(thread);
}

bool scheduler_idle(void)
{
    return current == &idle_thread;
}

uint32_t scheduler_get_quantum_remaining(void)
{
    return quantum_remaining;
}

uintptr_t scheduler_idle_stack_pointer(void)
{
    return idle_thread.kernel_sp;
}


