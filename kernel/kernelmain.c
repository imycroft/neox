#include "kernel.h"

int kernel_main(uint32_t magic,
                struct multiboot_info *mb_info)
{

    kernel_init(magic, mb_info);

    kernel_loop();

    return 0;
}
