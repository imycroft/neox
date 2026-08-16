#include "types.h"
#include "panic.h"
#include "kernel.h"
#include "arch.h"
#include "process.h"
#include "thread.h"
#include "reaper.h"
#include "assert.h"
#include "scheduler.h"

#include "tests.h"

#include "printf.h"

static void init_entry(void)
{
    printf("multiboot2_dump_module()\n");

    multiboot2_dump_module();
   // kernel_tests();
}

int kernel_main(uint32_t magic,
                struct multiboot_info *mb_info)
{
    struct process *process;
    struct thread *thread;

    kernel_init(magic, mb_info);

    /*
     * scheduler_start() intentionally left interrupts disabled.
     */

    /*
     * Start the reaper before creating any detached threads.
     */
    reaper_init();

    /*
     * Create the initial kernel process/thread.
     */
    process = process_create("init");

    ASSERT(process != NULL);

    thread = thread_create(process, init_entry);

    ASSERT(thread != NULL);

    /*
     * scheduler_add/thread_add requires interrupts to be disabled.
     * They already are disabled here.
     */
    thread_add(thread);

    /*
     * Transfer permanently from the bootstrap execution context
     * to the idle thread's dedicated kernel stack.
     *
     * This function never returns.
     */
    scheduler_enter_idle(scheduler_idle_stack_pointer());

    /*
     * Unreachable.
     */
    panic("scheduler_enter_idle returned");

    return 0;
}
