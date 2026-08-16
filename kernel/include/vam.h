#ifndef VAM_H
#define VAM_H

#include "memory_layout.h"
#include "memory.h"
#include "types.h"

/*
 * Kernel Virtual Address Manager.
 *
 * Manages virtual address space in the kernel region:
 *
 *     KERNEL_VIRT_BASE (0xC0000000) to KERNEL_VIRT_END (0xFFBFFFFF)
 *
 * User space and the recursive paging window are not managed here.
 */

void  vam_init(void);

void *vam_alloc_pages(uint32_t count);

void  vam_free_pages(uintptr_t addr, uint32_t count);

void *vam_alloc_stack(void);

void vam_free_stack(void *address);

void  vam_dump(void);

#endif
