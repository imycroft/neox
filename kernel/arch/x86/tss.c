#include "tss.h"
#include "gdt.h"
#include "string.h"

/* ------------------------------------------------------------------ */
/* Single global TSS                                                    */
/* ------------------------------------------------------------------ */

static struct tss_entry tss;

extern void tss_load(uint16_t selector);   /* tss_load.asm */

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void tss_init(void)
{
    memset(&tss, 0, sizeof(tss));

    /*
     * ss0 must point at the kernel data segment so the CPU can
     * restore DS/SS on privilege transitions.
     */
    tss.ss0       = GDT_DATA_SELECTOR;
    tss.esp0      = 0;   /* updated per-thread by tss_set_kernel_stack() */
    tss.iomap_base = sizeof(tss);  /* no I/O bitmap */

    /* Install the TSS descriptor into the GDT. */
    gdt_set_tss((uint32_t)&tss, sizeof(tss) - 1);

    /* Load the Task Register so the CPU can find the TSS. */
    tss_load(GDT_TSS_SELECTOR);
}

void tss_set_kernel_stack(uintptr_t esp0)
{
    tss.esp0 = esp0;
}
