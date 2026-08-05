#include "arch.h"

#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"

#define EFLAGS_IF (1u << 9)

interrupt_state_t interrupt_save(void)
{
    interrupt_state_t state;

    __asm__ volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "cli"
        : "=r"(state)
        :
        : "memory"
    );

    return state;
}

void interrupt_restore(interrupt_state_t state)
{
    __asm__ volatile(
        "push %0\n\t"
        "popf"
        :
        : "r"(state)
        : "memory", "cc"
    );
}

void interrupt_disable(void)
{
    __asm__ volatile(
        "cli"
        :
        :
        : "memory"
    );
}

void interrupt_enable(void)
{
    __asm__ volatile(
        "sti"
        :
        :
        : "memory"
    );
}

bool interrupt_enabled(void)
{
    interrupt_state_t state;

    __asm__ volatile(
        "pushf\n\t"
        "pop %0"
        : "=r"(state)
        :
        : "memory"
    );

    return (state & EFLAGS_IF) != 0;
}

void arch_init(void)
{
    gdt_init();

    idt_init();

    pic_init();

    pit_init(100);

    interrupt_enable();
}


