#include "isr.h"
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
    printf("\n=== CPU EXCEPTION ===\n");

    if (regs->int_no < 32)
        printf("%s\n", exception_messages[regs->int_no]);

    printf("Vector : %u\n", regs->int_no);
    printf("Error  : %x\n", regs->err_code);
    printf("EIP    : %x\n", regs->eip);

    while (1)
        __asm__ volatile ("hlt");
}
