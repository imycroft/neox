#pragma once

#include "types.h"
#include "multiboot2.h"

extern uint8_t kernel_end;
extern uintptr_t main_sp;

void kernel_init(uint32_t magic,
                 struct multiboot_info *mb_info);

void kernel_loop(void);
