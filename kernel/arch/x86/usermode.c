#include "usermode.h"

#include "assert.h"
#include "thread.h"
#include "process.h"
#include "tss.h"
#include "heap.h"
#include "memory.h"
#include "paging.h"
#include "pmm.h"
#include "scheduler.h"
#include "printf.h"
#include "util.h"



/* ------------------------------------------------------------------ */
/* Internal trampoline                                                  */
/* ------------------------------------------------------------------ */

/*
 * Per-thread descriptor held on the kernel heap.
 * The thread's kernel-thread entry points to usermode_trampoline()
 * which reads this and jumps into Ring 3.
 */
struct usermode_desc
{
    uintptr_t user_entry;   /* virtual address of the Ring-3 function */
    uintptr_t user_esp;     /* top of the mapped user stack           */
};

static void usermode_trampoline(void)
{
    struct thread       *thread = scheduler_current();
    struct usermode_desc *desc;

    ASSERT(thread != NULL);
    ASSERT(thread->usermode_desc != NULL);

    desc = (struct usermode_desc *)thread->usermode_desc;

    /*
     * Point the TSS at this thread's kernel stack top so that the
     * CPU can restore kernel context when Ring 3 is interrupted.
     *
     * kernel_sp is the current stack pointer just after the
     * cpu_context frame.  The top of the stack (where the CPU should
     * land after a ring transition) is the base + size.
     */
    uintptr_t kstack_top = (uintptr_t)thread->kernel_stack + KERNEL_STACK_SIZE;

    tss_set_kernel_stack(kstack_top);

    jump_usermode(desc->user_esp, desc->user_entry);

    /* jump_usermode() never returns */
}

static uintptr_t user_stack_alloc(struct process *process)
{
    uintptr_t virt;

    ASSERT(process != NULL);

    virt = USER_STACK_REGION_END - USER_STACK_SIZE;

    while (virt >= USER_STACK_REGION_START)
    {
        struct list_node *node;
        bool occupied;

        occupied = false;

        node = list_front(&process->user_stacks);

        while (node != &process->user_stacks.head)
        {
            struct user_stack *stack;

            stack = container_of(
                node,
                struct user_stack,
                node
            );

            if (stack->virt == virt)
                {
                    occupied = true;
                    break;
                }

            node = list_next(node);

        }

        /*
         * Also verify that this virtual page is not already mapped *
         * for another purpose in this process.
         */
        if (!occupied && paging_translate( process->page_directory, virt ) == 0)
        {
            return virt;
        }

        if (virt < USER_STACK_SIZE)
            break;

        virt -= USER_STACK_SIZE;
    }

    return 0;
}

static bool user_stack_reserve(
    struct process *process,
    uintptr_t virt)
{
    struct user_stack *stack;

    ASSERT(process != NULL);
    ASSERT(virt != 0);

    stack = kmalloc(sizeof(*stack));

    if (stack == NULL)
        return false;

    list_node_init(&stack->node);

    stack->virt = virt;

    list_push_back(
        &process->user_stacks,
        &stack->node
    );

    return true;
}

void usermode_stack_release(
    struct process *process,
    uintptr_t virt)
{
    struct list_node *node;

    ASSERT(process != NULL);
    ASSERT(virt != 0);

    node = list_front(&process->user_stacks);

    while (node != &process->user_stacks.head)
    {
        struct user_stack *stack;
        struct list_node *next;

        next = list_next(node);

        stack = container_of(
            node,
            struct user_stack,
            node
        );

        if (stack->virt == virt)
        {
            list_remove(&stack->node);
            kfree(stack);
            return;
        }

        node = next;
    }

    ASSERT(false);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

struct thread *usermode_thread_create(struct process *process,
                                      void           (*user_fn)(void))
{
    struct thread        *thread;
    struct usermode_desc *desc;
    void                 *phys;
    uintptr_t stack_virt;

    ASSERT(process != NULL);
    ASSERT(user_fn != NULL);
    ASSERT(process->page_directory != NULL);

    /*
     * Allocate the user-mode descriptor.
     *
     * The descriptor belongs to the thread and contains the information
     * required by the user-mode trampoline.
     */
    desc = kmalloc(sizeof(*desc));

    if (desc == NULL)
        return NULL;

    stack_virt = user_stack_alloc(process);

    if (stack_virt == 0)
    {
        kfree(desc);
        return NULL;
    }

    if (!user_stack_reserve(process, stack_virt))
    {
        kfree(desc); return NULL;

    }
    /*
     * Allocate the physical page backing the user stack.
     *
     * This page is owned by this process/thread and must be released if
     * any subsequent operation fails.
     */
    phys = pmm_alloc_page();

    if (phys == NULL)
    {
        kfree(desc);
        return NULL;
    }

    /*
     * Map the user stack into THIS PROCESS'S page directory.
     *
     * USER_STACK_VIRT is a user-space virtual address. It is therefore
     * not mapped through the shared kernel directory.
     *
     * Different processes may use the same virtual address because each
     * process has its own page directory:
     *
     *     process A: USER_STACK_VIRT -> physical page A
     *     process B: USER_STACK_VIRT -> physical page B
     *
     * PAGE_USER is required for Ring-3 access.
     */
   paging_map(
        process->page_directory,
        stack_virt,
        (uintptr_t)phys,
              PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
              );

    /*
     * Verify that the mapping was actually installed.
     *
     * paging_map() currently returns void, so explicitly verify the
     * resulting mapping before continuing.
     */
    ASSERT(
        paging_validate_mapping(
            process->page_directory,
            stack_virt
        )
    );

    /*
     * The user stack grows downward.
     *
     */
    desc->user_entry = (uintptr_t)user_fn;
    desc->user_esp   = stack_virt + USER_STACK_SIZE;

    /*
     * Create the kernel-side thread.
     *
     * The thread starts in usermode_trampoline(), which will eventually
     * transition the CPU from Ring 0 to Ring 3.
     */
    thread = thread_create(process, usermode_trampoline);

    if (thread == NULL)
    {
        /*
         * thread_create() failed, so release everything allocated by
         * this function.
         *
         * Recover the mapping before releasing the physical page.
         */
        paging_unmap(
            process->page_directory,
            stack_virt
        );

        pmm_free_page(phys);

        usermode_stack_release(
            process,
            stack_virt
        );

        kfree(desc);

        return NULL;
    }

    /*
     * Attach the user-mode descriptor to the newly created thread.
     */
    thread->usermode_desc = desc;

    thread->user_stack = stack_virt;
    thread->usermode_desc = desc;

    return thread;
}

/*
 * Create a Ring-3 thread for an already-loaded ELF image.
 *
 * The ELF loader is responsible for creating the process's user
 * address space and loading the program at its requested virtual
 * addresses.
 *
 * This function only provides:
 *
 *     1. A user-mode stack.
 *     2. A small kernel-side descriptor containing the ELF entry.
 *     3. A kernel thread whose trampoline performs the Ring-0
 *        -> Ring-3 transition.
 *
 * The existing usermode_thread_create() is intentionally left
 * untouched. It remains available for the original Ring-3 smoke
 * tests using kernel functions.
 */
struct thread * usermode_elf_thread_create(struct process *process,
                           uintptr_t entry)
{
    struct thread        *thread;
    struct usermode_desc *desc;
    void                 *phys;
    uintptr_t             stack_virt;

    ASSERT(process != NULL);

    /*
     * The ELF loader should give us a valid user-space entry point.
     *
     * We deliberately do not interpret the address as a C function
     * pointer. It is simply the virtual EIP at which the CPU will
     * begin executing the loaded ELF image.
     */
    if (entry == 0)
        return NULL;

    /*
     * Allocate the descriptor used by usermode_trampoline().
     */
    desc = kmalloc(sizeof(*desc));

    if (desc == NULL)
        return NULL;

    /*
     * Allocate the user stack.
     */
    stack_virt = user_stack_alloc(process);

    if (stack_virt == 0)
    {
        kfree(desc);
        return NULL;
    }


    if (!user_stack_reserve(process, stack_virt))
    {
        kfree(desc);
        return NULL;
    }

    /*
     * Allocate one physical page for the user stack.
     *
     * This page will be mapped into the process at
     * USER_STACK_VIRT with PAGE_USER, allowing Ring 3 to access it.
     */
    phys = pmm_alloc_page();

    if (phys == NULL)
    {
        usermode_stack_release(process, stack_virt);
        kfree(desc);
        return NULL;
    }

    /*
     * Map the physical page into the process address space.
     *
     * IMPORTANT:
     *
     * Unlike the old usermode_thread_create(), the ELF program
     * already lives in process->page_directory. The stack belongs
     * to that same address space.
     */
    paging_map(
        process->page_directory,
        stack_virt,
        (uintptr_t)phys,
               PAGE_PRESENT |
               PAGE_WRITABLE |
               PAGE_USER
    );

    ASSERT(
        paging_validate_mapping(
            process->page_directory, stack_virt
        )
    );

    /*
     * Store the ELF entry point and the initial user stack pointer.
     */
    desc->user_entry = entry;
    desc->user_esp = stack_virt + USER_STACK_SIZE;

    /*
     * Create the kernel-side thread.
     *
     * The thread initially executes usermode_trampoline() in Ring 0.
     * The trampoline then calls jump_usermode(), which performs the
     * actual IRET transition to Ring 3.
     */
    thread = thread_create(
        process,
        usermode_trampoline
    );

    if (thread == NULL)
    {
        paging_unmap(
            process->page_directory,
            stack_virt
        );

        pmm_free_page(phys);

        usermode_stack_release(process, stack_virt);

        kfree(desc);

        return NULL;
    }

    /*
     * Give the trampoline access to the descriptor.
     */
    thread->user_stack = stack_virt;
    thread->usermode_desc = desc;

    return thread;
}

bool usermode_stack_create(
    struct process *process,
    uintptr_t *user_esp)
{
    uintptr_t stack_virt;
    uintptr_t phys;

    ASSERT(process != NULL);
    ASSERT(user_esp != NULL);
    ASSERT(process->page_directory != NULL);

    stack_virt = user_stack_alloc(process);

    if (stack_virt == 0)
        return false;

    if (!user_stack_reserve(process, stack_virt))
        return false;

    phys = (uintptr_t)pmm_alloc_page();

    if (phys == 0)
    {
        usermode_stack_release(
            process,
            stack_virt
        );

        return false;
    }

    paging_map(
        process->page_directory,
        stack_virt,
        phys,
        PAGE_PRESENT |
        PAGE_WRITABLE |
        PAGE_USER
    );

    if (!paging_validate_mapping(
        process->page_directory,
        stack_virt))
    {
        paging_unmap(
            process->page_directory,
            stack_virt
        );

        pmm_free_page((void *)phys);

        usermode_stack_release(
            process,
            stack_virt
        );

        return false;
    }

    *user_esp = stack_virt + USER_STACK_SIZE;

    return true;
}
