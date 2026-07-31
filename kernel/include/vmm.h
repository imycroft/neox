#ifndef VMM_H
#define VMM_H

#include "memory.h"
#include "types.h"

void *vmm_alloc_page(uintptr_t virt);
void vmm_free_page(uintptr_t virt);

void *vmm_alloc_pages(uintptr_t virt, uint32_t count);
void vmm_free_pages(uintptr_t virt, uint32_t count);

/* Allocate anywhere in the kernel virtual address space. */


#endif
