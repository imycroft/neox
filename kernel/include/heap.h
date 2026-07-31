#ifndef HEAP_H
#define HEAP_H

#include "types.h"

void heap_init(void);

void heap_dump(void);

/*
 * Allocate 'size' bytes from the kernel heap.
 * Returns NULL on failure.
 */
void *kmalloc(uint32_t size);

void kfree(void *ptr);

#endif
