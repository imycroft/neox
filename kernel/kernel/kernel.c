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
#include "printf.h"


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

    /*
     * The calling context (this boot stack) becomes the idle
     * thread. From this point on, the timer interrupt is free
     * to preempt into any thread added to the scheduler.
     */
    scheduler_start();

    /* Filesystem */


}

void kernel_loop(void)
{

    printf("SHOULD NOT!\n");
    for (;;)
        __asm__ volatile("hlt");
}
