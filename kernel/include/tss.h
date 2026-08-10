#pragma once

#include "types.h"

/*
 * 32-bit Task State Segment.
 *
 * The CPU reads this on every privilege-level change (Ring 3 → Ring 0)
 * to find the kernel stack (ss0:esp0) to switch to.
 * We use a single, global TSS.
 */
struct tss_entry
{
    uint32_t prev_tss;   /* unused (no hardware task switching) */
    uint32_t esp0;       /* kernel stack pointer loaded on ring-3 → ring-0 */
    uint32_t ss0;        /* kernel stack segment                            */
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/*
 * tss_init() — install the TSS descriptor into the GDT and load TR.
 * Must be called after gdt_init().
 */
void tss_init(void);

/*
 * tss_set_kernel_stack() — update esp0 so that the next ring-3 → ring-0
 * transition lands on the correct kernel stack for the running thread.
 * Called by the scheduler every time it switches threads.
 */
void tss_set_kernel_stack(uintptr_t esp0);
