#include "types.h"
#include "kernel.h"
#include "tests.h"
#include "process.h"
#include "thread.h"
#include "arch.h"
#include "reaper.h"
#include "printf.h"
static void init_entry(void)
{
    kernel_tests();
}

int kernel_main(uint32_t magic,
                struct multiboot_info *mb_info)
{
    kernel_init(magic, mb_info);

    /*
     * Start the reaper kernel thread before any detached thread
     * can terminate.
     */
    reaper_init();

    /*
     * Spawn the init thread.  All test suites and any future
     * kernel work runs from here, not from the idle/boot stack.
     * This ensures that blocking calls (thread_join, mutex_lock,
     * wait_queue_sleep ...) always have a real thread to remove
     * from the ready list.
     */
    struct process *process = process_create("init");
    struct thread  *thread  = thread_create(process, init_entry);

    interrupt_state_t state = interrupt_save();
    thread_add(thread);
    interrupt_restore(state);

    /*
     * Drop into the idle loop.  The PIT will preempt into the
     * init thread on the first tick.
     */
    kernel_loop();

    return 0;
}
