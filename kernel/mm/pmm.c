#include "string.h"
#include "pmm.h"
#include "multiboot2.h"
#include "printf.h"
#include "kernel.h"
#include "arch.h"

/* Physical memory statistics */
static uint32_t total_memory;
static uint32_t usable_memory;
static uint64_t highest_address;
static uint32_t total_pages;

/* Bitmap information */
static uint8_t *bitmap;
static uintptr_t bitmap_address;
static uint32_t bitmap_size;
static uint32_t bitmap_pages;

static uint32_t free_pages;

static uint32_t fail_allocations;

static void bitmap_set(uint32_t page)
{
    bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(uint32_t page)
{
    bitmap[page / 8] &= (uint8_t)~(1 << (page % 8));
}

static int bitmap_test(uint32_t page)
{
    return (bitmap[page / 8] >> (page % 8)) & 1;
}

static void bitmap_clear_range(uint64_t addr, uint64_t len)
{
    uint32_t first_page;
    uint32_t pages;
    uint32_t i;

    first_page = (uint32_t)(addr / PAGE_SIZE);
    pages = (uint32_t)(len / PAGE_SIZE);

    for (i = 0; i < pages; i++)
        bitmap_clear(first_page + i);
}

static void bitmap_set_range(uint64_t addr, uint64_t len)
{
    uint32_t first_page;
    uint32_t pages;
    uint32_t i;

    first_page = (uint32_t)(addr / PAGE_SIZE);
    pages = (uint32_t)((len + PAGE_SIZE - 1) / PAGE_SIZE);

    for (i = 0; i < pages; i++)
        bitmap_set(first_page + i);
}

void pmm_init(void)
{
    const struct multiboot_tag_mmap *mmap;
    const struct multiboot_mmap_entry *entry;

    const struct multiboot_info *mb_info;
    const struct multiboot_tag_module *module;
    uintptr_t reserved_end;
    uintptr_t kernel_physical_end;
    uintptr_t mb_info_physical;

    uint32_t count;
    uint32_t i;

    total_memory = 0;
    usable_memory = 0;
    highest_address = 0;

    mmap = multiboot2_memory_map();
    mb_info = multiboot2_info();
    module = multiboot2_find_module("rootfs");

    if (mmap == NULL)
    {
        printf("PMM: no memory map\n");
        return;
    }


    count = (mmap->size - sizeof(*mmap)) / mmap->entry_size;

    /* First pass: collect memory statistics. */

    entry = mmap->entries;

    for (i = 0; i < count; i++)
    {
        total_memory += (uint32_t)entry->len;

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            uint64_t end = entry->addr + entry->len;

            usable_memory += (uint32_t)entry->len;


            if (end > highest_address)
                highest_address = end;
        }

        entry = (const struct multiboot_mmap_entry *)
        ((const uint8_t *)entry + mmap->entry_size);
    }


    total_pages = (uint32_t)((highest_address + PAGE_SIZE - 1) / PAGE_SIZE);

    bitmap_size = (total_pages + 7) / 8;

    kernel_physical_end = (uintptr_t)&kernel_end;


    reserved_end = kernel_physical_end;

    /* switching to high kernel made the mb_info virtual high-half as well (0xC), we need to convert it back
     * to physical before testing its position
    */
    mb_info_physical = VIRT_TO_PHYS(mb_info);

    if ((uintptr_t)mb_info_physical + mb_info->total_size > reserved_end)
        reserved_end = (uintptr_t)mb_info_physical + mb_info->total_size;

    bitmap_address = (reserved_end + PAGE_SIZE - 1)
    & ~(PAGE_SIZE - 1);

    bitmap = (uint8_t *)PHYS_TO_VIRT(bitmap_address);

    memset(bitmap, 0xFF, bitmap_size);
    //

    bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    printf("Total memory : %u KB\n", total_memory / 1024);
    printf("Usable memory: %u KB\n", usable_memory / 1024);
/*
    printf("Pages : %u\n", total_pages);
    printf("Bitmap: %u bytes\n", bitmap_size);

    printf("Bitmap addr : %x\n", (uint32_t)bitmap_address);
    printf("Bitmap pages: %u\n", bitmap_pages);*/

    /* Second pass: mark usable pages as free. */

    entry = mmap->entries;

    for (i = 0; i < count; i++)
    {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
            bitmap_clear_range(entry->addr, entry->len);

        entry = (const struct multiboot_mmap_entry *)
        ((const uint8_t *)entry + mmap->entry_size);
    }

    /* Reserve 0-1 MB */

    bitmap_set_range(0, 0x00100000);

    /* Reserve pages occupied by the kernel and the bitmap. */

    bitmap_set_range(
        KERNEL_LOAD_ADDRESS,
        kernel_physical_end - KERNEL_LOAD_ADDRESS
    );
    bitmap_set_range((uintptr_t)mb_info_physical,
                     mb_info->total_size);

    if (module != NULL)
    {
        bitmap_set_range(
            module->mod_start,
            module->mod_end - module->mod_start
        );
    }

    bitmap_set_range(bitmap_address,
                     bitmap_pages * PAGE_SIZE);

    free_pages = 0;

    for (i = 0; i < total_pages; i++)
    {
        if (!bitmap_test(i))
            free_pages++;
    }
}

void *pmm_alloc_page(void)
{
    uint32_t page;
    uintptr_t addr;

    interrupt_state_t state;

    if (fail_allocations != 0)
    {
        fail_allocations--;
        return NULL;
    }
    state = interrupt_save();

    for (page = 0; page < total_pages; page++)
    {
        if (!bitmap_test(page))
        {
            bitmap_set(page);
            free_pages--;

            addr = (uintptr_t)page * PAGE_SIZE;

            interrupt_restore(state);
            return (void *)addr;
        }
    }
    interrupt_restore(state);
    return NULL;
}

void pmm_free_page(void *page)
{
    uint32_t page_number;
    interrupt_state_t state;

    state = interrupt_save();

    page_number = (uint32_t)page / PAGE_SIZE;

    if (page_number >= total_pages)
    {
        interrupt_restore(state);
        return;
    }

    if (!bitmap_test(page_number))
    {
        interrupt_restore(state);
        return;
    }

    bitmap_clear(page_number);
    free_pages++;

    interrupt_restore(state);
}

uint32_t pmm_total_memory(void)
{
    return total_memory;
}

uint32_t pmm_usable_memory(void)
{
    return usable_memory;
}

uint32_t pmm_free_pages(void)
{
    return free_pages;
}

void pmm_fail_next_allocations(uint32_t count)
{
    fail_allocations = count;
}
