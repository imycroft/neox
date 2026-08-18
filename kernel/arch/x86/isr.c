#include "isr.h"

#include "scheduler.h"
#include "thread.h"

#include "panic.h"
#include "printf.h"

static const char *exception_messages[32] =
{
    "Divide Error",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

void isr_handler(struct registers *regs)
{
    uint32_t privilege;

    privilege = regs->cs & 0x3;

    printf("\n=== CPU EXCEPTION ===\n");

    if (regs->int_no < 32)
        printf("%s\n", exception_messages[regs->int_no]);

    printf("Vector : %u\n", regs->int_no);
    printf("Error  : %x\n", regs->err_code);
    printf("EIP    : %x\n", regs->eip);
    printf("CS     : %x\n", regs->cs);
    printf("CPL    : %u\n", privilege);

    if (regs->int_no == 14)
    {
        uint32_t cr2;

        __asm__ volatile ("mov %%cr2, %0"
        : "=r"(cr2));

        printf("CR2    : %x\n", cr2);
    }

    if (privilege == 3)
    {
        printf("USER MODE EXCEPTION\n");

        thread_kill_current();

        panic("user exception returned");
    }

    printf("KERNEL MODE EXCEPTION\n");

    while (1)
        __asm__ volatile ("hlt");
}
