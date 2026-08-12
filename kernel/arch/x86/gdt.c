#include "gdt.h"
#include "tss.h"

#include "printf.h"
/* ------------------------------------------------------------------ */
/* GDT descriptor layout (32-bit protected mode)                       */
/* ------------------------------------------------------------------ */

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;   /* [7:4] flags, [3:0] limit_high */
    uint8_t  base_high;
} __attribute__((packed));

/*
 * System descriptor (used for TSS) — same bit layout as a normal
 * descriptor but the access byte encodes a system type.
 */
struct gdt_system_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr   gdtr;

extern void gdt_load(struct gdt_ptr *gdtr);

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static void gdt_set_entry(uint32_t index,
                          uint32_t base,
                          uint32_t limit,
                          uint8_t  access,
                          uint8_t  flags)
{
    if (index >= GDT_ENTRIES)
        return;

    gdt[index].limit_low   = limit & 0xFFFF;
    gdt[index].base_low    = base  & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].access      = access;
    gdt[index].granularity = ((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/*
 * gdt_set_tss() — write the TSS descriptor (system descriptor, type 9)
 * into GDT slot GDT_TSS_ENTRY.  Called by tss_init() before ltr.
 *
 * Access byte for a 32-bit TSS (available): 0x89
 *   Present(1) | DPL=0(00) | S=0 | Type=1001 (32-bit TSS available)
 * Granularity flags: 0x00 (byte granularity, limit in bytes)
 */
void gdt_set_tss(uint32_t base, uint32_t limit)
{
    gdt_set_entry(GDT_TSS_ENTRY,
                  base,
                  limit,
                  0x89,   /* Present | DPL 0 | 32-bit TSS available */
                  0x00);  /* byte granularity */
}

void gdt_init(void)
{
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uintptr_t)gdt;

    /* 0 — Null descriptor */
    gdt_set_entry(GDT_NULL,        0, 0,       0x00, 0x00);

    /* 1 — Kernel code  (Ring 0, readable, execute) */
    gdt_set_entry(GDT_KERNEL_CODE, 0, 0xFFFFF, 0x9A, 0x0C);

    /* 2 — Kernel data  (Ring 0, writable)          */
    gdt_set_entry(GDT_KERNEL_DATA, 0, 0xFFFFF, 0x92, 0x0C);

    /*
     * 3 — User code  (Ring 3, readable, execute)
     * Access: Present(1) | DPL=3(11) | S=1 | Type=1010 = 0xFA
     */
    gdt_set_entry(GDT_USER_CODE,   0, 0xFFFFF, 0xFA, 0x0C);

    /*
     * 4 — User data  (Ring 3, writable)
     * Access: Present(1) | DPL=3(11) | S=1 | Type=0010 = 0xF2
     */
    gdt_set_entry(GDT_USER_DATA,   0, 0xFFFFF, 0xF2, 0x0C);

    /* 5 — TSS: written later by tss_init(), zero-init for now */
    gdt_set_entry(GDT_TSS_ENTRY,   0, 0,       0x00, 0x00);

    gdt_load(&gdtr);

}
