#include "string.h"
#include "pmm.h"
#include "paging.h"
#include "printf.h"
#include "panic.h"
#include "assert.h"

#define PAGE_FRAME_MASK 0xFFFFF000

static struct page_directory *kernel_directory;

// static struct page_table *kernel_table;

static struct page_table *paging_get_table(struct page_directory *directory, uint32_t directory_index)
{
    page_entry_t entry;

    entry = directory->entries[directory_index];

    if (!(entry & PAGE_PRESENT))
        return NULL;

    return (struct page_table *)(entry & PAGE_FRAME_MASK);
}

static bool paging_create_kernel_directory(void)
{
    kernel_directory = pmm_alloc_page();

    if (kernel_directory == NULL)
    {
        printf("Paging: page directory allocation failed\n");
        return false;
    }

    memset(kernel_directory, 0, sizeof(*kernel_directory));

    printf("Page directory : %x\n", (uint32_t)kernel_directory);

    return true;
}

static struct page_table *paging_alloc_table(void)
{
    struct page_table *table;

    table = pmm_alloc_page();

    if (table == NULL)
        return NULL;

    memset(table, 0, sizeof(*table));

    return table;
}

static void paging_identity_map(void)
{

    uint32_t i;

    for (i = 0; i < PAGE_ENTRIES; i++)
    {
        paging_map(kernel_directory,
                   i * PAGE_SIZE,
                   i * PAGE_SIZE,
                   PAGE_PRESENT | PAGE_WRITABLE);
    }
}

static void paging_enable_cpu(void)
{
    paging_load_directory((uint32_t)kernel_directory);
    printf("CR3 loaded\n");

    paging_enable();
    printf("Paging enabled\n");
}

struct page_directory *paging_get_kernel_directory(void)
{
    return kernel_directory;
}

void paging_init(void)
{
    if (!paging_create_kernel_directory())
        panic("Paging: failed to create kernel page directory");

    paging_identity_map();
    paging_enable_cpu();

    printf("Paging initialized\n");
}

void paging_map(struct page_directory *directory,
                uintptr_t virt,
                uintptr_t phys,
                uint32_t flags)
{
    struct page_table *table;
    uint32_t directory_index;
    uint32_t table_index;

    directory_index = virt >> 22;
    table_index = (virt >> 12) & 0x3FF;

    table = paging_get_table(directory, directory_index);

    if (table == NULL)
    {
        table = paging_alloc_table();

        if (table == NULL)
        {
            printf("paging_map: page table allocation failed\n");
            return;
        }

        directory->entries[directory_index] =
        (uint32_t)table |
        PAGE_PRESENT |
        (flags & (PAGE_WRITABLE | PAGE_USER));
    }

    table->entries[table_index] =
    (phys & PAGE_FRAME_MASK) | flags;

    paging_invalidate(virt);
}

void paging_unmap(struct page_directory *directory,
                  uintptr_t virt)
{
    uint32_t directory_index;
    uint32_t table_index;
    struct page_table *table;

    directory_index = virt >> 22;
    table_index = (virt >> 12) & 0x3FF;

    table = paging_get_table(directory, directory_index);

    if (table == NULL)
        return;

    table->entries[table_index] = 0;

    paging_invalidate(virt);
}

uintptr_t paging_translate(struct page_directory *directory,
                           uintptr_t virt)
{
    uint32_t directory_index;
    uint32_t table_index;
    uint32_t offset;

    struct page_table *table;
    page_entry_t entry;

    directory_index = virt >> 22;
    table_index = (virt >> 12) & 0x3FF;
    offset = virt & 0xFFF;

    table = paging_get_table(directory, directory_index);

    if (table == NULL)
        return 0;

    entry = table->entries[table_index];

    if (!(entry & PAGE_PRESENT))
        return 0;

    return (entry & PAGE_FRAME_MASK) + offset;
}

struct page_directory *paging_create_directory(void)
{
    struct page_directory *directory;

    directory = pmm_alloc_page();

    if (directory == NULL)
        return NULL;

    memset(directory, 0, sizeof(*directory));

    paging_copy_kernel_mappings(directory);

    return directory;
}

void paging_copy_kernel_mappings(struct page_directory *directory)
{
    uint32_t i;

    ASSERT(directory != NULL);
    ASSERT(kernel_directory != NULL);

    /*
     * Copy the kernel page-directory entries.
     *
     * The page tables themselves are shared. We only copy the
     * PDEs, so both directories reference the same physical
     * page tables.
     */
    for (i = 0; i < PAGE_ENTRIES; i++)
    {
        directory->entries[i] = kernel_directory->entries[i];
    }
}
