#ifndef HEAP_H
#define HEAP_H

#include "types.h"

void heap_init(void);

void heap_dump(void);

void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
