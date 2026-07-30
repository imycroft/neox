
#include "boot.h"

#include "multiboot2.h"

#include "printf.h"

void boot_init(uint32_t magic,
               struct multiboot_info *mb_info)
{
    multiboot2_init(magic, mb_info);

    //multiboot2_dump_memory_map();
}
