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

    paging_map(USER_STACK_VIRT,
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
        paging_unmap(USER_STACK_VIRT);
        pmm_free_page(phys);
        kfree(desc);
        return NULL;
    }

    thread->usermode_desc = desc;

    printf("[usermode] thread created: entry=%x user_esp=%x\n",
           desc->user_entry, desc->user_esp);

    return thread;
}
