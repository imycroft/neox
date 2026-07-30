#include "kernel.h"
#include "boot.h"
#include "arch.h"
#include "printf.h"
#include "keymap.h"
#include "drivers.h"
#include "console.h"
#include "pmm.h"
#include "paging.h"

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

    uintptr_t phys;

    paging_map(0x400000,
               0x200000,
               PAGE_PRESENT | PAGE_WRITABLE);

phys = paging_translate(0x400123);

    printf("Translated: %x\n", (uint32_t)phys);
}

void kernel_loop(void)
{
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}
