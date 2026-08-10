#include "syscall.h"
#include "idt.h"
#include "gdt.h"
#include "printf.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "arch.h"
#include "thread.h"

/* ------------------------------------------------------------------ */
/* Syscall dispatch                                                     */
/* ------------------------------------------------------------------ */

extern void syscall_stub(void);

/*
 * syscall_init() — register INT 0x80 with DPL=3 so Ring-3 code can
 * invoke `int 0x80` without triggering a GPF.
 *
 * Gate flags: 0xEE = Present(1) | DPL=3(11) | 0 | Gate-type=1110 (32-bit interrupt gate)
 */
void syscall_init(void)
{
    /*
     * We call idt_set_gate_dpl3() which is the same as idt_set_gate()
     * but uses 0xEE instead of 0x8E so user code can trigger it.
     */
    extern void idt_set_syscall_gate(uint8_t vector,
                                     uint32_t handler,
                                     uint16_t selector,
                                     uint8_t flags);

    idt_set_syscall_gate(0x80,
                         (uint32_t)syscall_stub,
                         GDT_CODE_SELECTOR,
                         0xEE);   /* DPL=3 interrupt gate */
}

/* ------------------------------------------------------------------ */
/* Handlers                                                             */
/* ------------------------------------------------------------------ */

static void sys_exit(int code)
{
    (void)code;

    struct thread *thread = scheduler_current();

    if (thread == NULL)
        return;

    interrupt_disable();

    scheduler_terminate(thread);
    scheduler_yield();

    /* unreachable */
}

static void sys_write(const char *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
        printf("%c", buf[i]);
}

void syscall_handler(struct registers *regs)
{
    /*
     * Calling convention (Linux-compatible for simplicity):
     *   EAX = syscall number
     *   EBX = arg1
     *   ECX = arg2
     *   EDX = arg3
     */
    uint32_t nr  = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;

    switch (nr)
    {
        case SYS_EXIT:
            sys_exit((int)arg1);
            break;

        case SYS_WRITE:
            sys_write((const char *)arg1, arg2);
            break;

        default:
            printf("[syscall] unknown syscall %u\n", nr);
            break;
    }
}
