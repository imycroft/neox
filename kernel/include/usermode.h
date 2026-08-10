#pragma once

#include "types.h"

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
