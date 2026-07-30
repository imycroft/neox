#pragma once

#include "multiboot2.h"

void boot_init(uint32_t magic,
               struct multiboot_info *mb_info);
