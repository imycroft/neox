#pragma once

#include "types.h"

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

/*
 * jump_usermode() — switch the current thread to Ring 3.
 *
 * @user_esp  : top of the user stack (virtual, Ring-3 accessible)
 * @user_entry: virtual address of the user-mode entry point
 *
 * This function does NOT return.  It performs an IRET into Ring 3
 * with EFLAGS.IF set so interrupts remain enabled in user mode.
 */
void jump_usermode(uintptr_t user_esp, uintptr_t user_entry);

/*
 * usermode_thread_create() — create a kernel thread that, when first
 * scheduled, immediately transitions to Ring 3 at @user_fn.
 *
 * The kernel thread owns the user stack; it is freed on process
 * destruction.  The returned thread has not been added to the
 * scheduler yet — call thread_add() as usual.
 */
struct thread;
struct process;

struct thread *usermode_thread_create(struct process *process,
                                      void (*user_fn)(void));

/*
 * Create a Ring-3 thread whose entry point is an ELF virtual address.
 *
 * Unlike usermode_thread_create(), this does not accept a kernel
 * function pointer. The entry address must already belong to the
 * process's user address space and must have been loaded by the ELF
 * loader.
 */
struct thread *usermode_elf_thread_create(
    struct process *process,
    uintptr_t entry
);
