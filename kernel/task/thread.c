#include "thread.h"
#include "assert.h"
#include "panic.h"
#include "paging.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "process.h"
#include "pmm.h"
#include "vam.h"

#include "arch/x86/context.h"
#include "arch.h"

#include "heap.h"
#include "string.h"
#include "wait.h"
#include "printf.h"
#include "usermode.h"

// Private functions
static void thread_exit(void)
{
    thread_kill_current();
}

/*
 * Initial entry point for every kernel thread.
 *
 * A thread's first run is reached via context_switch()'s
 * `ret`, not `iret`. When the switch was triggered from
 * inside the timer interrupt handler, interrupts are still
 * disabled at this point (the ISR's `cli` was never undone
 * by a matching `iret`), so it must be re-enabled explicitly
 * here for preemption of this thread to begin.
 */
static void thread_bootstrap(void)
{
    struct thread *thread;

    interrupt_enable();

    thread = scheduler_current();

    thread->entry();

    thread_exit();
}

static uint32_t next_tid = 1;

// API Functions

void thread_add(struct thread *thread)
{
    ASSERT(thread != NULL);

    list_push_back(&thread->process->threads,
                   &thread->group_node);

    scheduler_add(thread);
}
void thread_block(void)
{
    interrupt_state_t state;

    ASSERT(scheduler_current() != NULL);

    state = interrupt_save();

    struct thread *thread = scheduler_current();

    thread->state = THREAD_BLOCKED;
    scheduler_remove(thread);
    scheduler_yield();

    interrupt_restore(state);
}

void thread_unblock(struct thread *thread)
{
    interrupt_state_t state;

    ASSERT(thread != NULL);
    ASSERT(thread->state == THREAD_BLOCKED);

    state = interrupt_save();

    thread->state = THREAD_READY;

    scheduler_add(thread);

    interrupt_restore(state);
}

struct thread *thread_create(
    struct process *process,
    void (*entry)(void)
)
{
    struct thread *thread;
    uintptr_t      virt;
    void           *phys;
    uint32_t        stack_pages;
    uint32_t        i;
    uintptr_t      *stack;
    struct cpu_context *context;

    ASSERT(process != NULL);

    thread = kmalloc(sizeof(*thread));

    if (thread == NULL)
        return NULL;

    memset(thread, 0, sizeof(*thread));

    list_node_init(&thread->group_node);
    list_node_init(&thread->sched_node);
    list_node_init(&thread->wait_node);
    list_node_init(&thread->zombie_node);

    wait_queue_init(&thread->termination_queue);

    thread->wait_queue    = NULL;
    thread->detached      = false;
    thread->usermode_desc = NULL;

    thread->entry = entry;

    /*
     * Allocate a virtual address slot from the dedicated kernel-stack
     * region.
     *
     * The virtual slot itself does not allocate physical memory.
     * Physical pages are allocated and mapped below.
     */
    thread->kernel_stack = vam_alloc_stack();

    if (thread->kernel_stack == NULL)
    {
        kfree(thread);
        return NULL;
    }

    /*
     * Allocate physical pages and map them into the kernel-stack slot.
     */
    stack_pages = KERNEL_STACK_SIZE / PAGE_SIZE;

    for (i = 0; i < stack_pages; i++)
    {
        phys = pmm_alloc_page();

        if (phys == NULL)
        {
            /*
             * Roll back all pages that were successfully allocated
             * before the failure.
             */
            while (i != 0)
            {
                i--;

                virt = (uintptr_t)thread->kernel_stack +
                (uintptr_t)i * PAGE_SIZE;

                /*
                 * Recover the physical address before removing
                 * the mapping.
                 */
                phys = (void *)paging_translate(
                    paging_get_kernel_directory(),
                                                virt
                );

                paging_unmap(
                    paging_get_kernel_directory(),
                             virt
                );

                if (phys != NULL)
                    pmm_free_page(phys);
            }

            vam_free_stack(thread->kernel_stack);
            thread->kernel_stack = NULL;

            kfree(thread);
            return NULL;
        }

        virt = (uintptr_t)thread->kernel_stack +
        (uintptr_t)i * PAGE_SIZE;

        paging_map_kernel(
            virt,
            (uintptr_t)phys,
                          PAGE_PRESENT | PAGE_WRITABLE
        );

        ASSERT(
            paging_validate_mapping(
                paging_get_kernel_directory(),
                                    virt
            )
        );


        ASSERT(
            paging_validate_mapping(
                process->page_directory,
                virt
            )
        );
    }
    /*
     * Start at the top of the virtual kernel stack.
     *
     * The stack grows downward.
     */

    uintptr_t stack_phys;
    uintptr_t stack_top;

    /*
     * Get the physical address of the last byte of the stack.
     *
     * The stack grows downward, so we initialize from the top page.
     */
    stack_top =
    (uintptr_t)thread->kernel_stack +
    KERNEL_STACK_SIZE;

    /*
     * Translate the virtual stack top to physical memory.
     *
     * Subtract one because stack_top is the first byte AFTER the stack.
     */
    stack_phys =
    paging_translate(
        paging_get_kernel_directory(),
                     stack_top - sizeof(uintptr_t)
    );

    if (stack_phys == 0)
    {
        /*
         * This should never happen because the stack was mapped above.
         */
        vam_free_stack(thread->kernel_stack);
        kfree(thread);
        return NULL;
    }


    stack = (uintptr_t *)stack_top;

    /*
     * Build the initial stack frame.
     */
    *--stack = (uintptr_t)thread_exit;
    *--stack = (uintptr_t)thread_bootstrap;



    /*
     * Reserve space for the callee-saved registers restored by
     * context_switch():
     *
     *     pop edi
     *     pop esi
     *     pop ebx
     *     pop ebp
     */
    context = (struct cpu_context *)(stack - 4);

    memset(context, 0, sizeof(*context));


    thread->kernel_sp = (uintptr_t)context;

    thread->state = THREAD_READY;

    thread->tid = next_tid++;
    thread->process = process;
    // printf("thread %u created\n", thread->tid);
    return thread;
}

void thread_destroy(struct thread *thread)
{
    uint32_t   stack_pages;
    uint32_t   i;
    uintptr_t  virt;
    void      *phys;

    printf("destroying thread %s\n", thread->process->name);
    ASSERT(thread != NULL);
    ASSERT(!interrupt_enabled());
    ASSERT(thread->state == THREAD_TERMINATED);

    /*
     * Remove the thread from its owning process.
     *
     * Stack-allocated test stubs may never have been added via
     * thread_add(), so only remove the node when it is linked.
     */
    if (thread->group_node.prev != NULL &&
        thread->group_node.next != NULL)
    {
        list_remove(&thread->group_node);
    }

    /*
     * Release the physical pages backing the kernel stack.
     *
     * kernel_stack is now a virtual address returned by vam_alloc_stack(),
     * not a heap allocation.
     */
    if (thread->kernel_stack != NULL)
    {
        stack_pages = KERNEL_STACK_SIZE / PAGE_SIZE;

        for (i = 0; i < stack_pages; i++)
        {
            virt = (uintptr_t)thread->kernel_stack +
            (uintptr_t)i * PAGE_SIZE;

            /*
             * Get the physical frame before removing the mapping.
             */
            phys = (void *)paging_translate(
                paging_get_kernel_directory(),
                                            virt
            );

            paging_unmap(
                paging_get_kernel_directory(),
                         virt
            );

            if (phys != NULL)
                pmm_free_page(phys);
        }

        /*
         * The virtual stack slot can now be reused by another thread.
         */
        vam_free_stack(thread->kernel_stack);

        thread->kernel_stack = NULL;
    }

    /*
     * Release the physical page backing the user-mode stack.
     *
     * The user stack belongs to the process address space, so its mapping
     * must be removed from the process page directory, not the kernel
     * directory.
     */
    if (thread->usermode_desc != NULL)
    {
        phys = (void *)paging_translate(
            thread->process->page_directory,
            USER_STACK_VIRT
        );

        paging_unmap(
            thread->process->page_directory,
            USER_STACK_VIRT
        );

        if (phys != NULL)
            pmm_free_page(phys);

        kfree(thread->usermode_desc);
        thread->usermode_desc = NULL;
    }

    kfree(thread);
}

void thread_join(struct thread *thread)
{
    interrupt_state_t state;
    struct process   *process;

    ASSERT(thread != NULL);
    ASSERT(!thread->detached);

    state = interrupt_save();

    if (thread->state != THREAD_TERMINATED)
    {
        wait_queue_sleep(&thread->termination_queue);
    }


    /*
     * Thread is now THREAD_TERMINATED and off the scheduler.
     * We are the sole owner (joinable, not detached), so it is
     * safe to destroy it here.
     */
    process = thread->process;

    thread_destroy(thread);

    if (process != NULL && list_empty(&process->threads))
        process_destroy(process);

    interrupt_restore(state);
}

void thread_detach(struct thread *thread)
{
    interrupt_state_t state;

    ASSERT(thread != NULL);
    ASSERT(!thread->detached);

    state = interrupt_save();

    thread->detached = true;

    /*
     * If the thread already terminated before we called detach,
     * scheduler_terminate() did not push it (detached was false
     * at the time).  Do it now.
     */
    if (thread->state == THREAD_TERMINATED)
        scheduler_push_zombie(thread);

    interrupt_restore(state);
}

void thread_yield(void)
{
    if (scheduler_current() == NULL)
        return;

    interrupt_state_t state;

    state = interrupt_save();

    scheduler_yield();

    interrupt_restore(state);
}

void thread_kill_current(void)
{
    struct thread *thread;

    interrupt_disable();

    thread = scheduler_current();

    ASSERT(thread != NULL);

    scheduler_terminate(thread);
    printf("destroying thread %s\n", thread->process->name); // __debug
    scheduler_yield();

    panic("terminated thread resumed");
}
