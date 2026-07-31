#pragma once

#include "memory.h"
#include "types.h"

void pmm_init(void);

void *pmm_alloc_page(void);

void pmm_free_page(void *page);

uint32_t pmm_total_memory(void);

uint32_t pmm_usable_memory(void);

uint32_t pmm_free_pages(void);
