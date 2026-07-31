#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define PAGE_SIZE               4096

#define KERNEL_IDENTITY_SIZE    (4 * 1024 * 1024)

#define VAM_START               KERNEL_IDENTITY_SIZE

#define ADDRESS_SPACE_SIZE      (4ULL * 1024 * 1024 * 1024)

#define TOTAL_VIRTUAL_PAGES     (ADDRESS_SPACE_SIZE / PAGE_SIZE)

#define VAM_BITMAP_SIZE         (TOTAL_VIRTUAL_PAGES / 8)


#endif
