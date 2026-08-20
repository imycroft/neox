#include "syscall.h"
#include "assert.h"
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
#include "exec.h"
#include "string.h"
#include "elf_loader.h"
#include "file.h"
#include "usermode.h"

#define SYSCALL_ERROR ((uint32_t)-1)
/* ------------------------------------------------------------------ */
/* Syscall dispatch                                                     */
/* ------------------------------------------------------------------ */

extern void syscall_stub(void);

static bool exec_build_user_stack(
    struct exec_args *args,
    uintptr_t stack_virt,
    uintptr_t stack_phys,
    uintptr_t *user_esp)
{
    uintptr_t stack_low;
    uintptr_t stack_high;
    uintptr_t esp;
    uintptr_t argv_virt;
    uint32_t i;
    uint32_t strings_size;
    uint32_t pointers_size;
    uint32_t total_size;
    uint8_t *stack;

    if (args == NULL)
        return false;

    if (user_esp == NULL)
        return false;

    stack_low = stack_virt;
    stack_high = stack_virt + USER_STACK_SIZE;

    stack = (uint8_t *)PHYS_TO_VIRT(stack_phys);

    /*
     * Calculate the amount of space required for the strings.
     */
    strings_size = 0;

    for (i = 0; i < (uint32_t)args->argc; i++)
    {
        uint32_t length;

        length = strlen(args->argv[i]) + 1;

        if (strings_size > UINT32_MAX - length)
            return false;

        strings_size += length;
    }

    /*
     * argc + argv pointers + NULL terminator.
     */
    pointers_size =
    sizeof(uint32_t) +
    ((uint32_t)args->argc + 1) * sizeof(uint32_t);

    total_size = pointers_size + strings_size;

    /*
     * Keep the initial stack 16-byte aligned.
     */
    if (total_size > USER_STACK_SIZE)
        return false;

    esp = stack_high;

    /*
     * Copy argument strings from kernel memory into the physical
     * page backing the new user stack.
     *
     * We cannot use the user virtual address here because CR3 still
     * points to the old address space.
     */
    esp -= strings_size;

    {
        uintptr_t strings_virt;
        uint32_t offset;

        strings_virt = esp;
        offset = 0;

        for (i = 0; i < (uint32_t)args->argc; i++)
        {
            uint32_t length;

            length = strlen(args->argv[i]) + 1;

            memcpy(
                stack + (strings_virt - stack_low) + offset,
                   args->argv[i],
                   length
            );

            offset += length;
        }
    }

    /*
     * Reserve space for argv[].
     *
     * argv will contain user virtual addresses pointing into the
     * strings above.
     */
    esp -= ((uint32_t)args->argc + 1) * sizeof(uint32_t);

    argv_virt = esp;

    /*
     * Store argv pointers.
     */
    {
        uint32_t string_offset;

        string_offset = 0;

        for (i = 0; i < (uint32_t)args->argc; i++)
        {
            uint32_t *argv_entry;

            argv_entry =
            (uint32_t *)(stack +
            (argv_virt - stack_low) +
            i * sizeof(uint32_t));

            *argv_entry =
            (uint32_t)(
                (stack_high - strings_size) +
                string_offset
            );

            string_offset += strlen(args->argv[i]) + 1;
        }

        /*
         * argv[argc] = NULL
         */
        *(uint32_t *)(stack +
        (argv_virt - stack_low) +
        (uint32_t)args->argc * sizeof(uint32_t)) = 0;
    }

    /*
     * Push argc immediately below argv.
     */
    esp -= sizeof(uint32_t);

    *(uint32_t *)(stack + (esp - stack_low)) =
    (uint32_t)args->argc;

    /*
     * Align the final stack pointer.
     */
    esp &= ~0xFu;

    /*
     * Move the stack down if alignment crossed the data we created.
     */
    if (esp < stack_low)
        return false;

    *user_esp = esp;

    return true;
}

static inline uintptr_t read_cr3(void)
{
    uintptr_t value;

    __asm__ volatile (
        "mov %%cr3, %0"
        : "=r"(value)
    );

    return value;
}

struct syscall_frame
{
    struct registers regs;

    /*
     * These two values are pushed by the CPU when INT 0x80
     * crosses from Ring 3 to Ring 0.
     */
    uint32_t user_esp;
    uint32_t user_ss;
};

static uint32_t *syscall_user_esp_slot(
    struct registers *regs)
{
    return ((uint32_t *)regs) + 17;
}

static uint32_t *syscall_user_ss_slot(
    struct registers *regs)
{
    return ((uint32_t *)regs) + 18;
}

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
    printf(
        "[EXIT] process=%s status=%u\n",
        scheduler_current()->process->name,
           code
    );

    thread_kill_current();

    panic("thread_kill_current returned");
}
static uint32_t sys_write(int fd, const char *buf, uint32_t len)
{

    struct thread *thread;
    struct process *process;
    uint32_t i;

    thread = scheduler_current();

    if (thread == NULL)
        return SYSCALL_ERROR;

    process = thread->process;

    if (process == NULL)
        return SYSCALL_ERROR;

    /*
     * For now, only support stdout.
     *
     * Later this should be replaced by the actual
     * file descriptor/device implementation.
     */

    if (fd != 1)
        return SYSCALL_ERROR;

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

static int32_t sys_exec(
    struct registers *regs,
    const char *path,
    char *const argv[])
{
    struct thread *thread;
    struct process *process;
    struct exec_args args;

    struct file file;

    struct page_directory *old_directory;
    struct page_directory *new_directory;

    const void *image;
    uint32_t image_size;

    uintptr_t entry;
    uintptr_t user_esp;
    uintptr_t stack_virt;
    uintptr_t stack_phys;

    thread = scheduler_current();

    if (thread == NULL)
        return -1;

    process = thread->process;

    if (process == NULL)
        return -1;

    /*
     * The syscall arguments belong to the OLD address space.
     *
     * Validate and copy everything we need before replacing it.
     */
    if (!paging_user_string_valid(
        process->page_directory,
        path))
    {
        return -1;
    }

    if (exec_args_copy_from_user(
        &args,
        process->page_directory,
        (const char *const *)argv) != 0)
    {
        return -1;
    }

    /*
     * Open the executable while the old address space is still
     * active.
     */
    if (file_open(
        path,
        FILE_ACCESS_READ,
        &file) != 0)
    {
        exec_args_destroy(&args);
        return -1;
    }

    image = file_data(&file);
    image_size = file_size(&file);

    if (image == NULL || image_size == 0)
    {
        file_close(&file);
        exec_args_destroy(&args);
        return -1;
    }

    old_directory = process->page_directory;

    /*
     * Construct the completely new address space.
     */
    new_directory = paging_create_directory();

    if (new_directory == NULL)
    {
        file_close(&file);
        exec_args_destroy(&args);
        return -1;
    }

    /*
     * Load the ELF directly into the NEW address space.
     *
     * The old address space is still active in CR3.
     */
    if (!elf_load(
        new_directory,
        image,
        image_size,
        &entry))
    {
        paging_destroy_directory(new_directory);

        file_close(&file);
        exec_args_destroy(&args);

        return -1;
    }

    /*
     * Temporarily attach the new directory to the process so the
     * user-stack allocator operates on the new address space.
     */
    process->page_directory = new_directory;

    /*
     * Create the new Ring-3 stack.
     */
    if (!usermode_stack_create(
        process,
        &user_esp))
    {
        process->page_directory = old_directory;

        paging_destroy_directory(new_directory);

        file_close(&file);
        exec_args_destroy(&args);

        return -1;
    }

    /*
     * The stack allocator selected the virtual address immediately
     * below USER_STACK_REGION_END.
     *
     * Recover the base from the returned top address.
     */
    stack_virt = user_esp - USER_STACK_SIZE;

    ASSERT(
        paging_validate_mapping(
            new_directory,
            stack_virt
        )
    );
    /*
     * Translate the new stack through the NEW page directory.
     *
     * We are still executing with the OLD CR3, so we cannot access
     * stack_virt directly.
     */
    stack_phys = paging_translate(
        new_directory,
        stack_virt
    );

    ASSERT(stack_phys != 0);

    if (stack_phys == 0)
    {
        process->page_directory = old_directory;

        paging_destroy_directory(new_directory);

        file_close(&file);
        exec_args_destroy(&args);

        return -1;
    }

    /*
     * Build argc/argv in the physical page backing the new user
     * stack.
     *
     * The new virtual address space is not active yet.
     */
    if (!exec_build_user_stack(
        &args,
        stack_virt,
        stack_phys,
        &user_esp))
    {
        process->page_directory = old_directory;

        paging_destroy_directory(new_directory);

        file_close(&file);
        exec_args_destroy(&args);

        return -1;
    }

    /*
     * Everything required by the new image now exists.
     *
     * From this point onward exec() commits.
     *
     * We switch CR3 while the kernel code and kernel stack remain
     * accessible because the kernel mappings are shared.
     */

    ASSERT(
        paging_user_range_valid(
            new_directory,
            stack_virt,
            USER_STACK_SIZE
        )
    );

    uintptr_t new_directory_phys;

    new_directory_phys =
    VIRT_TO_PHYS((uintptr_t)new_directory);

    ASSERT(new_directory_phys != 0);

    paging_load_directory(
        VIRT_TO_PHYS(
            (uintptr_t)new_directory
        )
    );

    ASSERT(read_cr3() == new_directory_phys);
    /*
     * The process now permanently owns the new address space.
     */
    process->page_directory = new_directory;

    /*
     * The old user address space is no longer needed.
     *
     * paging_destroy_directory() only destroys the user portion
     * (PDEs 0-767), leaving the shared kernel mappings untouched.
     */
    paging_destroy_directory(old_directory);

    ASSERT(
        paging_user_range_valid(
            new_directory,
            stack_virt,
            USER_STACK_SIZE
        )
    );

    /*
     * The current thread continues running.
     *
     * We do NOT create a new thread.
     *
     * We simply change the user context that the syscall's IRET
     * will restore.
     */


    ASSERT(regs != NULL);

    ASSERT(
        *syscall_user_ss_slot(regs) == GDT_USER_DATA_SELECTOR
    );

    *syscall_user_esp_slot(regs) = (uint32_t)user_esp;

    /*
     * The new program starts with exec()'s return value replaced by
     * the new program's entry point context, so there is no successful
     * return from exec().
     */

    regs->eip = (uint32_t)entry;

    *syscall_user_esp_slot(regs) = (uint32_t)user_esp;

    regs->eax = 0;


    file_close(&file);
    exec_args_destroy(&args);

    return 0;
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
            regs->eax = (uint32_t)sys_write(
                (int)arg1,
                (const char *)arg2,
                arg3
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

        case SYS_EXEC:
            regs->eax = (uint32_t)sys_exec(
                regs,
                (const char *)arg1,
                (char *const *)arg2
            );
            break;

        default:
            regs->eax = SYSCALL_ERROR;
            break;
    }
}
