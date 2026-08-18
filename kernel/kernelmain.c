#include "assert.h"
#include "types.h"
#include "panic.h"
#include "kernel.h"
#include "arch.h"
#include "process.h"
#include "reaper.h"
#include "scheduler.h"
#include "init.h"
#include "tests.h"

static void kernel_test_entry(void)
{
    //kernel_tests();
}

int kernel_main(uint32_t magic,
                struct multiboot_info *mb_info)
{
    struct process *process;
    struct thread *thread;
    kernel_init(magic, mb_info);

    /*
     * Start the reaper before creating any detached threads.
     */
    reaper_init();

    process = process_create("kernel_tests");

    ASSERT(process != NULL);

    thread = thread_create(process, kernel_test_entry);

    ASSERT(thread != NULL);

    /*
     * scheduler_add/thread_add requires interrupts to be disabled.
     * They already are disabled here.
     */
    thread_add(thread);

    init_process_start();

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
