#include "syscall.h"
#include "idt.h"
#include "gdt.h"
#include "paging.h"
#include "panic.h"
#include "printf.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "arch.h"
#include "thread.h"
#include "process.h"

#define SYSCALL_ERROR ((uint32_t)-1)
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

static void sys_exit(uint32_t code)
{
    (void)code;

    thread_kill_current();

    panic("thread_kill_current returned");
}
static uint32_t sys_write(const char *buf, uint32_t len)
{
    struct thread *thread;
    struct process *process;
    uint32_t i;

    thread = scheduler_current();

    if (thread == NULL)
        return SYSCALL_ERROR;

    process = thread->process;

    if (!paging_user_range_valid(
        process->page_directory,
        (uintptr_t)buf,
                                 len))
    {
        return SYSCALL_ERROR;
    }

    for (i = 0; i < len; i++)
        printf("%c", buf[i]);

    return len;
}

static int32_t sys_open(
    const char *path,
    uint32_t access)
{
    struct thread *thread;
    struct process *process;

    thread = scheduler_current();

    if (thread == NULL)
        return -1;

    process = thread->process;

    if (process == NULL)
        return -1;

    if (!paging_user_string_valid(
        process->page_directory,
        path))
    {
        return -1;
    }

    return process_open(process, path, access);
}

static int32_t sys_read(
    int fd,
    void *buffer,
    uint32_t count)
{
    struct thread *thread;
    struct process *process;

    thread = scheduler_current();

    if (thread == NULL)
        return -1;

    process = thread->process;

    if (process == NULL)
        return -1;

    if (!paging_user_range_valid(
        process->page_directory,
        (uintptr_t)buffer,
                                 count))
    {
        return -1;
    }

    return (int32_t)process_read(
        process,
        fd,
        buffer,
        count
    );
}

static int32_t sys_close(int fd)
{
    struct thread *thread;
    struct process *process;

    thread = scheduler_current();

    if (thread == NULL)
        return -1;

    process = thread->process;

    if (process == NULL)
        return -1;

    return process_close(process, fd);
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
    uint32_t arg3 = regs->edx;

    switch (nr)
    {
        case SYS_EXIT:
            sys_exit(arg1);
            break;

        case SYS_WRITE:
            regs->eax = sys_write(
                (const char *)arg1,
                arg2
            );
            break;

        case SYS_OPEN:
            regs->eax = (uint32_t)sys_open(
                (const char *)arg1,
                                           arg2
            );
            break;

        case SYS_READ:
            regs->eax = (uint32_t)sys_read(
                (int)arg1,
                                           (void *)arg2,
                                           arg3
            );
            break;

        case SYS_CLOSE:
            regs->eax = (uint32_t)sys_close(
                (int)arg1
            );
            break;

        default:
            regs->eax = SYSCALL_ERROR;
            break;
    }
}
