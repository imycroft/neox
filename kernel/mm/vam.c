#include "vam.h"
#include "memory_layout.h"
#include "memory.h"
#include "string.h"
#include "printf.h"

/*
 * ============================================================================
 * Kernel Virtual Address Manager
 * ============================================================================
 *
 * The VAM manages the kernel's virtual address space:
 *
 *     KERNEL_VIRT_BASE  (0xC0000000)
 *         to
 *     KERNEL_VIRT_END   (0xFFBFFFFF)
 *
 * It does NOT manage:
 *
 *     - user virtual address space (0x00000000-0xBFFFFFFF)
 *       → managed per-process by the future user VAM
 *
 *     - the recursive paging window (0xFFC00000-0xFFFFFFFF)
 *       → permanently reserved; not available for allocation
 *
 * The bitmap is indexed relative to KERNEL_VIRT_BASE:
 *
 *     bitmap[0] → 0xC0000000
 *     bitmap[1] → 0xC0001000
 *     ...
 *
 * A set bit means the page is OCCUPIED (reserved or allocated).
 * A clear bit means the page is FREE.
 *
 * vam_init() marks all fixed kernel regions as occupied so that
 * dynamic allocators cannot hand out addresses that belong to them.
 *
 * ============================================================================
 */


/*
 * ============================================================================
 * Bitmap sizing
 * ============================================================================
 *
 * The kernel virtual region spans KERNEL_VIRT_SIZE bytes.
 *
 * Each page is PAGE_SIZE (4096) bytes.
 *
 * Total pages:
 *
 *     KERNEL_VIRT_SIZE / PAGE_SIZE
 *     = 0x3FC00000 / 0x1000
 *     = 261120 pages
 *
 * Bitmap bytes:
 *
 *     261120 / 8 = 32640 bytes
 */

#define VAM_TOTAL_PAGES  (KERNEL_VIRT_SIZE / PAGE_SIZE)
#define VAM_BITMAP_BYTES ((VAM_TOTAL_PAGES + 7u) / 8u)


/*
 * ============================================================================
 * Bitmap storage
 * ============================================================================
 *
 * Statically allocated. The VAM does not depend on the heap or PMM.
 */

static uint8_t bitmap[VAM_BITMAP_BYTES];


/*
 * ============================================================================
 * Address / index conversion
 * ============================================================================
 *
 * A virtual address maps to a bitmap index as follows:
 *
 *     index = (virt - KERNEL_VIRT_BASE) / PAGE_SIZE
 *
 * And back:
 *
 *     virt = KERNEL_VIRT_BASE + index * PAGE_SIZE
 */

static inline uint32_t vam_addr_to_index(uintptr_t virt)
{
    return (uint32_t)((virt - KERNEL_VIRT_BASE) / PAGE_SIZE);
}

static inline uintptr_t vam_index_to_addr(uint32_t index)
{
    return KERNEL_VIRT_BASE + (uintptr_t)index * PAGE_SIZE;
}


/*
 * ============================================================================
 * Bitmap primitives
 * ============================================================================
 */

static void bitmap_set(uint32_t index)
{
    bitmap[index / 8u] |= (1u << (index % 8u));
}

static void bitmap_clear(uint32_t index)
{
    bitmap[index / 8u] &= ~(1u << (index % 8u));
}

static bool bitmap_test(uint32_t index)
{
    return (bitmap[index / 8u] & (1u << (index % 8u))) != 0;
}

static void bitmap_range_set(uint32_t start, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
        bitmap_set(start + i);
}

static void bitmap_range_clear(uint32_t start, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
        bitmap_clear(start + i);
}

static bool bitmap_range_free(uint32_t start, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        if (bitmap_test(start + i))
            return false;
    }

    return true;
}


/*
 * ============================================================================
 * Region reservation helpers
 * ============================================================================
 *
 * vam_reserve_region() marks all pages in [start, end] as occupied.
 *
 * Both addresses are inclusive virtual addresses within the kernel region.
 */

static void vam_reserve_region(uintptr_t start, uintptr_t end)
{
    uint32_t first;
    uint32_t count;

    first = vam_addr_to_index(start);
    count = (uint32_t)((end - start) / PAGE_SIZE) + 1u;

    bitmap_range_set(first, count);
}


/*
 * ============================================================================
 * vam_init()
 * ============================================================================
 *
 * Initializes the kernel VAM.
 *
 * All pages start FREE (bitmap cleared).
 *
 * Fixed kernel regions are then marked OCCUPIED:
 *
 *   1. Kernel image
 *
 *      From KERNEL_IMAGE_BASE to the page-aligned end of the kernel image.
 *
 *      The exact end comes from the linker symbol kernel_virtual_end,
 *      which is page-aligned up to the nearest page boundary.
 *
 *   2. Kernel heap region
 *
 *      KERNEL_HEAP_START to KERNEL_HEAP_END.
 *
 *      The heap allocator manages this region internally. The VAM marks
 *      the entire region occupied so it cannot be handed out as general
 *      virtual address space.
 *
 *   3. Kernel stacks region
 *
 *      KERNEL_STACKS_START to KERNEL_STACKS_END.
 *
 *      Same rationale as the heap region.
 */

void vam_init(void)
{
    extern uintptr_t kernel_virtual_end;

    uintptr_t image_end;

    /*
     * Start with all pages free.
     */
    memset(bitmap, 0, sizeof(bitmap));

    /*
     * 1. Reserve the kernel image.
     *
     * kernel_virtual_end is provided by the linker script and gives the
     * virtual address of the first byte after the kernel image (.bss end).
     *
     * Round up to the next page boundary so that the reservation covers
     * any partial trailing page.
     */
    image_end = ((uintptr_t)&kernel_virtual_end + PAGE_SIZE - 1u)
                & ~((uintptr_t)(PAGE_SIZE - 1u));

    vam_reserve_region(KERNEL_IMAGE_BASE, image_end - 1u);

    printf("VAM: kernel image reserved %x - %x\n",
           (uint32_t)KERNEL_IMAGE_BASE,
           (uint32_t)(image_end - 1u));

    /*
     * 2. Reserve the kernel heap region.
     */
    vam_reserve_region(KERNEL_HEAP_START, KERNEL_HEAP_END);

    printf("VAM: heap region reserved  %x - %x\n",
           (uint32_t)KERNEL_HEAP_START,
           (uint32_t)KERNEL_HEAP_END);

    /*
     * 3. Reserve the kernel stacks region.
     */
    vam_reserve_region(KERNEL_STACKS_START, KERNEL_STACKS_END);

    printf("VAM: stack region reserved %x - %x\n",
           (uint32_t)KERNEL_STACKS_START,
           (uint32_t)KERNEL_STACKS_END);

    /*
     * The recursive paging window (0xFFC00000-0xFFFFFFFF) is not part
     * of the VAM's range (KERNEL_VIRT_END = 0xFFBFFFFF), so it requires
     * no explicit reservation here.
     */

    printf("VAM: initialized (%u kernel pages, %u bytes bitmap)\n",
           VAM_TOTAL_PAGES, VAM_BITMAP_BYTES);
}


/*
 * ============================================================================
 * vam_alloc_pages()
 * ============================================================================
 *
 * Finds a contiguous run of 'count' free pages in the kernel virtual
 * address space and marks them occupied.
 *
 * Returns the virtual address of the first page, or NULL on failure.
 *
 * The search starts at KERNEL_VIRT_BASE (index 0) and scans upward.
 * In practice the fixed regions (image, heap, stacks) are marked occupied
 * at init time, so the first free run will be found in KERNEL_REMAINING.
 */

void *vam_alloc_pages(uint32_t count)
{
    uint32_t index;

    if (count == 0)
        return NULL;

    for (index = 0; index <= VAM_TOTAL_PAGES - count; index++)
    {
        if (bitmap_range_free(index, count))
        {
            bitmap_range_set(index, count);

            return (void *)vam_index_to_addr(index);
        }
    }

    return NULL;
}


/*
 * ============================================================================
 * vam_free_pages()
 * ============================================================================
 *
 * Marks 'count' pages starting at virtual address 'addr' as free.
 *
 * The caller is responsible for ensuring that the address and count
 * correspond to a previously allocated range.
 */

void vam_free_pages(uintptr_t addr, uint32_t count)
{
    uint32_t index;

    if (count == 0)
        return;

    index = vam_addr_to_index(addr);

    bitmap_range_clear(index, count);
}


/*
 * ============================================================================
 * vam_dump()
 * ============================================================================
 *
 * Debug: print the occupancy of pages around the kernel region boundaries.
 */

void vam_dump(void)
{
    uint32_t  i;
    uintptr_t addr;

    printf("==== VAM dump ====\n");

    /*
     * Show the first few pages of each fixed region so the reservations
     * can be visually verified.
     */

    printf("  [image base]\n");
    for (i = vam_addr_to_index(KERNEL_IMAGE_BASE);
         i < vam_addr_to_index(KERNEL_IMAGE_BASE) + 4;
         i++)
    {
        addr = vam_index_to_addr(i);
        printf("    %x : %s\n", (uint32_t)addr,
               bitmap_test(i) ? "OCCUPIED" : "free");
    }

    printf("  [heap start]\n");
    for (i = vam_addr_to_index(KERNEL_HEAP_START);
         i < vam_addr_to_index(KERNEL_HEAP_START) + 4;
         i++)
    {
        addr = vam_index_to_addr(i);
        printf("    %x : %s\n", (uint32_t)addr,
               bitmap_test(i) ? "OCCUPIED" : "free");
    }

    printf("  [stacks start]\n");
    for (i = vam_addr_to_index(KERNEL_STACKS_START);
         i < vam_addr_to_index(KERNEL_STACKS_START) + 4;
         i++)
    {
        addr = vam_index_to_addr(i);
        printf("    %x : %s\n", (uint32_t)addr,
               bitmap_test(i) ? "OCCUPIED" : "free");
    }

    printf("  [remaining start]\n");
    for (i = vam_addr_to_index(KERNEL_REMAINING_START);
         i < vam_addr_to_index(KERNEL_REMAINING_START) + 4;
         i++)
    {
        addr = vam_index_to_addr(i);
        printf("    %x : %s\n", (uint32_t)addr,
               bitmap_test(i) ? "OCCUPIED" : "free");
    }

    printf("==================\n");
}
