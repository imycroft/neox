#pragma once

#include "types.h"
#include "memory_layout.h"

/* Entries per table */

#define PAGE_ENTRIES 1024

#define PAGE_FRAME_MASK      0xFFFFF000u

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
void paging_invalidate(uintptr_t address);

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

struct page_directory *paging_get_kernel_directory(void);

struct page_directory *paging_create_directory(void);

void paging_map(struct page_directory *directory,
                uintptr_t virt,
                uintptr_t phys,
                uint32_t flags);

void paging_unmap(struct page_directory *directory,
                  uintptr_t virt);

uintptr_t paging_translate(struct page_directory *directory,
                           uintptr_t virt);

void paging_copy_kernel_mappings(struct page_directory *directory);

void paging_map_kernel(
    uintptr_t virt,
    uintptr_t phys,
    uint32_t flags
);

bool paging_user_range_valid(
    struct page_directory *directory,
    uintptr_t              addr,
    uint32_t               len);
