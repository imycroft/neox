#pragma once

#include "types.h"

/*
 * GDT descriptor indices.
 *
 * Index 0 : null descriptor
 * Index 1 : kernel code  (Ring 0, selector 0x08)
 * Index 2 : kernel data  (Ring 0, selector 0x10)
 * Index 3 : user   code  (Ring 3, selector 0x18 | 3 = 0x1B)
 * Index 4 : user   data  (Ring 3, selector 0x20 | 3 = 0x23)
 * Index 5 : TSS           (selector 0x28)
 */
#define GDT_ENTRIES 6

/* Flat selectors (RPL = 0) */
#define GDT_NULL_SELECTOR        0x00
#define GDT_CODE_SELECTOR        0x08
#define GDT_DATA_SELECTOR        0x10

/* User-mode selectors (RPL = 3 — OR'd in so the CPU checks ring 3) */
#define GDT_USER_CODE_SELECTOR   0x1B   /* 0x18 | 3 */
#define GDT_USER_DATA_SELECTOR   0x23   /* 0x20 | 3 */

/* TSS selector (RPL = 0) */
#define GDT_TSS_SELECTOR         0x28

enum
{
    GDT_NULL        = 0,
    GDT_KERNEL_CODE = 1,
    GDT_KERNEL_DATA = 2,
    GDT_USER_CODE   = 3,
    GDT_USER_DATA   = 4,
    GDT_TSS_ENTRY   = 5,
};

void gdt_init(void);

/*
 * gdt_set_tss() — write a system/TSS descriptor into slot GDT_TSS_ENTRY.
 * Called by tss_init() after the TSS has been prepared.
 */
void gdt_set_tss(uint32_t base, uint32_t limit);
