#include "string.h"
#include "pmm.h"
#include "paging.h"
#include "memory_layout.h"
#include "printf.h"
#include "panic.h"
#include "assert.h"

/*
 * ============================================================================
 * Constants
 * ============================================================================
 */

#define PAGE_FRAME_MASK      0xFFFFF000u

/*
 * ============================================================================
 * Kernel page directory
 * ============================================================================
 *
 * This is the authoritative kernel page directory.
 *
 * It is created by paging_init() after paging is already enabled by the
 * bootstrap (kernel.asm).
 *
 * Every process page directory copies PDEs KERNEL_PDE_START–KERNEL_PDE_END
 * from this directory so that kernel mappings are shared across all
 * address spaces.
 */

static struct page_directory *kernel_directory;


/*
 * ============================================================================
 * Page-table access
 * ============================================================================
 *
 * paging_get_table() extracts the physical frame address of a page table
 * from a PDE and converts it to a virtual address using PHYS_TO_VIRT().
 *
 * This is valid as long as the physical frame falls within the kernel's
 * higher-half direct mapping (i.e. within the first 16 MiB of physical
 * memory, which is covered by the bootstrap and later the permanent kernel
 * mappings).
 *
 * When recursive paging is introduced (step 7), this function will be
 * replaced with the recursive virtual-window approach.
 */

static struct page_table *paging_get_table(
    struct page_directory *directory,
    uint32_t               directory_index)
{
    page_entry_t entry;
    uintptr_t    phys;

    entry = directory->entries[directory_index];

    if (!(entry & PAGE_PRESENT))
        return NULL;

    phys = entry & PAGE_FRAME_MASK;

    /*
     * Convert the physical frame address to the corresponding virtual
     * address within the higher-half direct mapping.
     *
     * This is safe for page tables allocated via pmm_alloc_page(), which
     * returns physical addresses in the range covered by the bootstrap
     * identity + higher-half mapping.
     */
    return (struct page_table *)PHYS_TO_VIRT(phys);
}


/*
 * ============================================================================
 * Page-table allocation
 * ============================================================================
 */

static struct page_table *paging_alloc_table(void)
{
    struct page_table *table;
    uintptr_t phys;

    phys = (uintptr_t)pmm_alloc_page();

    if (phys == 0)
        return NULL;

    table = (struct page_table *)PHYS_TO_VIRT(phys);

    memset(table, 0, sizeof(*table));

    return table;
}


/*
 * ============================================================================
 * paging_init()
 * ============================================================================
 *
 * Called from kernel_init() after paging has already been enabled by the
 * bootstrap (kernel.asm).
 *
 * Responsibilities:
 *
 *   1. Allocate a proper kernel page directory.
 *   2. Copy the bootstrap kernel mappings (PDEs 768-771) into it.
 *   3. Install the recursive mapping at PDE 1023.
 *   4. Reload CR3 with the new directory's PHYSICAL address.
 *
 * The bootstrap identity mapping (PDEs 0-3) is intentionally preserved
 * at this stage. It will be removed in a later step once all kernel
 * subsystems have transitioned to higher-half virtual addresses.
 */

void paging_init(void)
{
    uintptr_t phys;

    /*
     * Allocate the kernel page directory.
     *
     * pmm_alloc_page() returns a physical address. We need both:
     *
     *   - the virtual address, to write into the directory
     *   - the physical address, to load into CR3
     */
    phys = (uintptr_t)pmm_alloc_page();

    if (phys == 0)
        panic("paging_init: failed to allocate kernel page directory");

    kernel_directory = (struct page_directory *)PHYS_TO_VIRT(phys);

    memset(kernel_directory, 0, sizeof(*kernel_directory));

    printf("Paging: kernel directory physical=%x virtual=%x\n",
           (uint32_t)phys,
           (uint32_t)kernel_directory);

    /*
     * Copy the bootstrap page directory into the new kernel directory.
     *
     * The bootstrap (kernel.asm) mapped the first 16 MiB of physical
     * memory at both:
     *
     *     0x00000000 (identity,    PDEs 0-3)
     *     0xC0000000 (higher-half, PDEs 768-771)
     *
     * CR3 currently holds the physical address of the bootstrap page
     * directory.  We read CR3 to obtain that address, convert it to a
     * virtual address through the active higher-half mapping, and copy
     * all 1024 PDEs into the new directory.
     *
     * The identity PDEs (0-3) are preserved for now. They will be
     * removed after all kernel subsystems operate exclusively through
     * higher-half virtual addresses.
     */
    {
        uint32_t               cr3;
        uint32_t               i;
        struct page_directory *bootstrap;

        // 2. Read CR3 with proper clobbers so the compiler doesn't reuse registers
        __asm__ volatile(
            "mov %%cr3, %0"
            : "=r"(cr3)
            :
            : "memory"
        );

        bootstrap = (struct page_directory *)PHYS_TO_VIRT((uintptr_t)cr3);

        for(i = 0; i < 1024; i++)
                kernel_directory->entries[i] = bootstrap->entries[i];
    }


    /*
     * Install the recursive mapping at PDE 1023.
     *
     * PDE 1023 points to the page directory's own physical address.
     *
     * This makes the virtual range 0xFFC00000-0xFFFFFFFF a window into
     * the page directory and all page tables.
     *
     * In particular:
     *
     *     0xFFFFF000 = virtual address of the page directory itself
     *     0xFFC00000 + (table_index * 0x1000) = virtual address of page table N
     */
    kernel_directory->entries[RECURSIVE_PDE_INDEX] =
        phys | PAGE_PRESENT | PAGE_WRITABLE;

    printf("Paging: recursive PDE installed at index %u\n",
           RECURSIVE_PDE_INDEX);

    /*
     * Reload CR3 with the physical address of the new page directory.
     *
     * This replaces the bootstrap page directory with the proper kernel
     * page directory while keeping the same virtual mappings active.
     */
    paging_load_directory((uint32_t)phys);

    printf("Paging: initialized\n");
}


/*
 * ============================================================================
 * Public API
 * ============================================================================
 */

struct page_directory *paging_get_kernel_directory(void)
{
    return kernel_directory;
}

void paging_map(struct page_directory *directory,
                uintptr_t              virt,
                uintptr_t              phys,
                uint32_t               flags)
{
    struct page_table *table;
    uintptr_t          table_phys;
    uint32_t           directory_index;
    uint32_t           table_index;

    directory_index = virt >> 22;
    table_index     = (virt >> 12) & 0x3FFu;

    table = paging_get_table(directory, directory_index);

    if (table == NULL)
    {
        table = paging_alloc_table();

        if (table == NULL)
        {
            printf("paging_map: page table allocation failed\n");
            return;
        }

        /*
         * The PDE must store the PHYSICAL address of the page table,
         * not the virtual one we are using to write to it.
         */
        table_phys = VIRT_TO_PHYS((uintptr_t)table);

        directory->entries[directory_index] =
            (uint32_t)table_phys |
            PAGE_PRESENT |
            (flags & (PAGE_WRITABLE | PAGE_USER));
    }

    table->entries[table_index] =
        (phys & PAGE_FRAME_MASK) | flags;

    paging_invalidate(virt);
}

void paging_unmap(struct page_directory *directory,
                  uintptr_t              virt)
{
    uint32_t           directory_index;
    uint32_t           table_index;
    struct page_table *table;

    directory_index = virt >> 22;
    table_index     = (virt >> 12) & 0x3FFu;

    table = paging_get_table(directory, directory_index);

    if (table == NULL)
        return;

    table->entries[table_index] = 0;

    paging_invalidate(virt);
}

uintptr_t paging_translate(struct page_directory *directory,
                           uintptr_t              virt)
{
    uint32_t           directory_index;
    uint32_t           table_index;
    uint32_t           offset;
    struct page_table *table;
    page_entry_t       entry;

    directory_index = virt >> 22;
    table_index     = (virt >> 12) & 0x3FFu;
    offset          = virt & 0xFFFu;

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
    uintptr_t              phys;

    phys = (uintptr_t)pmm_alloc_page();

    if (phys == 0)
        return NULL;

    directory = (struct page_directory *)PHYS_TO_VIRT(phys);

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
     * Copy kernel PDEs (768-1023, inclusive of the recursive PDE).
     *
     * The page tables themselves are shared — we copy only the PDEs,
     * so both directories reference the same physical page tables.
     *
     * User PDEs (0-767) are left zeroed.
     */
    for (i = KERNEL_PDE_START; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        directory->entries[i] = kernel_directory->entries[i];
    }
}
