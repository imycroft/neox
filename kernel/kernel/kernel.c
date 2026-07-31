#include "kernel.h"
#include "boot.h"
#include "arch.h"
#include "printf.h"
#include "keymap.h"
#include "drivers.h"
#include "console.h"
#include "pmm.h"
#include "paging.h"
#include "vmm.h"
#include "heap.h"
void kernel_init(uint32_t magic,
                 struct multiboot_info *mb_info)
{

    console_init();

    boot_init(magic, mb_info);


    drivers_init();
    printf("Drivers initialized\n");

    arch_init();
    printf("Architecture initialized\n");

    pmm_init();
    printf("PMM initialized\n");

    paging_init();

    heap_init();

    // Tests go here

    void *ptr[200];
    uint32_t i;

    for (i = 0; i < 200; i++)
    {
        ptr[i] = kmalloc(32);

        if (ptr[i] == NULL)
            break;
    }

    printf("Allocated %u blocks\n", i);

    heap_dump();

    // (void)a;
    // (void)b;
    // (void)c;
    (void)ptr;

    // End of tests
}

void kernel_loop(void)
{
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}
