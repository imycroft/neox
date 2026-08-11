#include "arch.h"

#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "tss.h"
#include "syscall.h"
#include "printf.h"
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

/*
 * Re-enable interrupts and immediately halt the CPU.
 *
 * The x86 guarantee that `sti` delays its effect by one
 * instruction means the `hlt` is always reached before any
 * pending interrupt is delivered.  This closes the race where
 * an interrupt could fire between a plain `sti` and a
 * subsequent `hlt`, causing the CPU to sleep past a wakeup.
 */
void interrupt_enable_and_halt(void)
{
    __asm__ volatile(
        "sti\n\t"
        "hlt"
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
    printf("after gdt_init\n");
    /* TSS must be installed after GDT so gdt_set_tss() can write slot 5 */
    tss_init();
    printf("after tss_init\n");
    idt_init();
    printf("after idt_init\n");
    /* Register INT 0x80 with DPL=3 so Ring-3 code can invoke syscalls */
    syscall_init();
    printf("after syscall_init\n");
    pic_init();
    printf("after pic_init\n");
    pit_init(100);
    printf("after pit_init\n");
    interrupt_enable();
    printf("after interrupt_enable\n");
}
