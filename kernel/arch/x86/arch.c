#include "arch.h"

#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"

void arch_init(void)
{
    gdt_init();

    idt_init();

    pic_init();

    pit_init(100);

    __asm__ volatile("sti");
}
