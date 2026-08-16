#include "usermode.h"
#include "thread.h"
#include "process.h"
#include "tss.h"
#include "heap.h"
#include "memory.h"
#include "paging.h"
#include "pmm.h"
#include "scheduler.h"
#include "printf.h"
#include "assert.h"

/*
 * Size of the user stack allocated for each user-mode thread.
 * One page is enough for our demo; a real OS would lazily grow it.
 */
#define USER_STACK_PAGES 1
#define USER_STACK_SIZE  (USER_STACK_PAGES * PAGE_SIZE)

/*
 * Virtual base address for the user stack.
 * We place it just below 3 GiB, well away from the kernel.
 * A real OS would give each process its own address space; here
 * we keep the single shared page directory and just pick a
 * deterministic address that is not in use.
 */
#define USER_STACK_VIRT  0xBFFFF000u   /* 3 GiB - 4 KiB */

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
    uintptr_t kstack_top =
    (uintptr_t)thread->kernel_stack + THREAD_STACK_SIZE;

    tss_set_kernel_stack(kstack_top);

    jump_usermode(desc->user_esp, desc->user_entry);

    /* jump_usermode() never returns */
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

    ASSERT(process != NULL);
    ASSERT(user_fn != NULL);

    /* Allocate the descriptor */
    desc = kmalloc(sizeof(*desc));
    if (desc == NULL)
        return NULL;

    /*
     * Allocate one physical page and identity-map it as user-accessible.
     * PAGE_USER lets Ring-3 code read/write this page without a GPF.
     */
    phys = pmm_alloc_page();
    if (phys == NULL)
    {
        kfree(desc);
        return NULL;
    }

    paging_map(paging_get_kernel_directory(),
               USER_STACK_VIRT,
               (uintptr_t)phys,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    /*
     * The stack grows downward; start the user ESP at the very top of
     * the page (one byte past the last valid address so that the first
     * `push` lands on the last dword of the page).
     */
    desc->user_entry = (uintptr_t)user_fn;
    desc->user_esp   = USER_STACK_VIRT + USER_STACK_SIZE;

    /* Create the kernel thread that will run the trampoline */
    thread = thread_create(process, usermode_trampoline);
    if (thread == NULL)
    {
        paging_unmap(paging_get_kernel_directory(), USER_STACK_VIRT);
        pmm_free_page(phys);
        kfree(desc);
        return NULL;
    }

    thread->usermode_desc = desc;

    printf("[usermode] thread created: entry=%x user_esp=%x\n",
           desc->user_entry, desc->user_esp);

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
struct thread *
usermode_elf_thread_create(struct process *process,
                           uintptr_t entry)
{
    struct thread        *thread;
    struct usermode_desc *desc;
    void                 *phys;

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
     * Allocate one physical page for the user stack.
     *
     * This page will be mapped into the process at
     * USER_STACK_VIRT with PAGE_USER, allowing Ring 3 to access it.
     */
    phys = pmm_alloc_page();

    if (phys == NULL)
    {
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
        USER_STACK_VIRT,
        (uintptr_t)phys,
               PAGE_PRESENT |
               PAGE_WRITABLE |
               PAGE_USER
    );

    /*
     * Store the ELF entry point and the initial user stack pointer.
     */
    desc->user_entry = entry;
    desc->user_esp   = USER_STACK_VIRT + USER_STACK_SIZE;

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
            USER_STACK_VIRT
        );

        pmm_free_page(phys);
        kfree(desc);

        return NULL;
    }

    /*
     * Give the trampoline access to the descriptor.
     */
    thread->usermode_desc = desc;

    printf(
        "[usermode ELF] entry=%x esp=%x stack phys=%x\n",
        (uint32_t)desc->user_entry,
           (uint32_t)desc->user_esp,
           (uint32_t)paging_translate(
               process->page_directory,
               USER_STACK_VIRT
           )
    );

    return thread;
}
