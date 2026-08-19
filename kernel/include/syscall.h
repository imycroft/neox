#pragma once

#include "types.h"
#include "isr.h"

#include "syscall_abi.h"

/*
 * syscall_init() — register INT 0x80 in the IDT with DPL=3 so
 * user-mode code can invoke it.
 */
void syscall_init(void);

/* Called from the INT 0x80 assembly stub. */
void syscall_handler(struct registers *regs);
