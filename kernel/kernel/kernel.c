#include "kernel.h"
#include "keymap.h"
#include "console.h"
#include "boot.h"
#include "drivers.h"
#include "arch.h"
#include "pmm.h"
#include "paging.h"
#include "vam.h"
#include "heap.h"
#include "scheduler.h"
#include "process.h"
#include "thread.h"
#include "arch/x86/context.h"

#include "printf.h"

// TESTING
uintptr_t main_sp;

static void thread1(void)
{
    printf("Thread 1 started\n");

    printf("Switching back to kernel\n");

    context_switch(
        &scheduler_current()->kernel_sp,
                   main_sp
    );

    printf("Back inside thread\n");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }

    printf("ERROR: returned to thread\n");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}
// END TESTING

void kernel_init(uint32_t magic,
                 struct multiboot_info *mb_info)
{

    /* Console */
    console_init();

    /* Boot */
    boot_init(magic, mb_info);


    /* Hardware */
    drivers_init();
    arch_init();


    /* Memory */
    pmm_init();
    paging_init();
    vam_init();
    heap_init();

    /* Tasking */
    scheduler_init();

    // TESTING

    struct process *process;
    struct thread *thread;

    process = process_create();

    if (process == NULL)
    {
        printf("Process creation failed\n");
        return;
    }

    thread = thread_create(process, thread1);

    if (thread == NULL)
    {
        printf("Thread creation failed\n");
        return;
    }

    scheduler_add(thread);
    scheduler_next();

    context_switch(&main_sp,
                   thread->kernel_sp);

    printf("Current thread : %x\n",
           (uint32_t)scheduler_current());

    printf("main_sp        : %x\n",
           (uint32_t)main_sp);
    uint32_t a;
    uint32_t b;
    uint32_t c;

    a = 10;
    b = 20;
    c = a + b;

    printf("Kernel resumed: %u\n", c);

    context_switch(&main_sp,
                   thread->kernel_sp);


    // END TESTING
    /* Filesystem */


}

void kernel_loop(void)
{
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}
