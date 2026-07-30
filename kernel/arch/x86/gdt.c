#include "gdt.h"

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;

    uint8_t base_middle;
    uint8_t access;

    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdtr;

extern void gdt_load(struct gdt_ptr *gdtr);

static void gdt_set_entry(uint32_t index,
                          uint32_t base,
                          uint32_t limit,
                          uint8_t access,
                          uint8_t flags)
{
    if (index >= GDT_ENTRIES)
        return;
    gdt[index].limit_low = limit & 0xFFFF;

    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].access = access;

    gdt[index].granularity =
    ((limit >> 16) & 0x0F) |
    ((flags & 0x0F) << 4);
}

void gdt_init(void)
{
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uintptr_t)gdt;

    /* Null descriptor */
    gdt_set_entry(
        GDT_NULL,
        0,
        0,
        0,
        0
    );

    /* Kernel code */
    gdt_set_entry(
        GDT_KERNEL_CODE,
        0,
        0xFFFFF,
        0x9A,           // Present | Ring 0 | Code | Readable
        0x0C
    );

    /* Kernel data */
    gdt_set_entry(
        GDT_KERNEL_DATA,
        0,
        0xFFFFF,
        0x92,           // Present | Ring 0 | Data | Writable
        0x0C            // 4 KiB granularity | 32-bit segment
    );

    gdt_load(&gdtr);
}
