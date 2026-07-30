#pragma once

#include "types.h"

/* Page size */

#define PAGE_SIZE 4096

/* Entries per table */

#define PAGE_ENTRIES 1024

/* Page table / directory entry flags */

#define PAGE_PRESENT   (1 << 0)
#define PAGE_WRITABLE  (1 << 1)
#define PAGE_USER      (1 << 2)
#define PAGE_PWT       (1 << 3)
#define PAGE_PCD       (1 << 4)
#define PAGE_ACCESSED  (1 << 5)
#define PAGE_DIRTY     (1 << 6)
#define PAGE_4MB       (1 << 7)
#define PAGE_GLOBAL    (1 << 8)

void paging_load_directory(uint32_t directory);
void paging_enable(void);
/* Page entry */

typedef uint32_t page_entry_t;

/* Page table */

struct page_table
{
    page_entry_t entries[PAGE_ENTRIES];
};

/* Page directory */

struct page_directory
{
    page_entry_t entries[PAGE_ENTRIES];
};

void paging_init(void);

void paging_map(uintptr_t virt,
                uintptr_t phys,
                uint32_t flags);

void paging_unmap(uintptr_t virt);

uintptr_t paging_translate(uintptr_t virt);

