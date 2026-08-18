#ifndef HEAP_H
#define HEAP_H

#include "types.h"

#define HEAP_ALIGNMENT 8u

void heap_init(void);

void heap_dump(void);

/*
 * Allocate 'size' bytes from the kernel heap.
 * Returns NULL on failure.
 */
void *kmalloc(uint32_t size);

void kfree(void *ptr);

#endif
