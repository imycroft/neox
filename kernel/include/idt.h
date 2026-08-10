#pragma once

#include "types.h"

#define IDT_ENTRIES 256
#define IDT_FLAG_INTERRUPT_GATE 0x8E

void idt_init(void);

/*
 * Install a gate with an explicit flags byte (e.g. 0xEE for DPL=3).
 * Used by syscall_init() to register INT 0x80 callable from Ring 3.
 */
void idt_set_syscall_gate(uint8_t  vector,
                          uint32_t handler,
                          uint16_t selector,
                          uint8_t  flags);
