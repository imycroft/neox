#pragma once

#include "types.h"

#define GDT_ENTRIES 3

#define GDT_NULL_SELECTOR 0x00
#define GDT_CODE_SELECTOR 0x08
#define GDT_DATA_SELECTOR 0x10

enum
{
    GDT_NULL,
    GDT_KERNEL_CODE,
    GDT_KERNEL_DATA
};

void gdt_init(void);
