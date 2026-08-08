#pragma once

#include "types.h"

struct registers
{
    /* Pushed by common_isr_stub */
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Pushed by the ISR stub */
    uint32_t int_no;
    uint32_t err_code;

    /* Pushed automatically by the CPU */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

void isr_handler(struct registers *regs);
