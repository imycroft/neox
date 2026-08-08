#pragma once

#include "types.h"

#define IDT_ENTRIES 256
#define IDT_FLAG_INTERRUPT_GATE 0x8E

void idt_init(void);
