#include "vmm.h"
#include "paging.h"
#include "pmm.h"
#include "vam.h"
#include "printf.h"

// Private functions

static void vmm_unmap_pages(uintptr_t virt,
                            uint32_t count)
{
    uint32_t i;

    struct page_directory *directory;

    directory = paging_get_kernel_directory();

    for (i = 0; i < count; i++)
    {
        uintptr_t phys;

        phys = paging_translate(directory, virt + i * PAGE_SIZE);

        if (phys == 0)
            continue;

        paging_unmap(directory, virt + i * PAGE_SIZE);

        pmm_free_page(
            (void *)(phys & ~(PAGE_SIZE - 1))
        );
    }
}

static bool vmm_map_pages(uintptr_t virt,
                          uint32_t count)
{
    uint32_t i;

    struct page_directory *directory;

    directory = paging_get_kernel_directory();

    for (i = 0; i < count; i++)
    {
        if (paging_translate(directory, virt + i * PAGE_SIZE) != 0)
        {
            printf("VMM: already mapped virt=%x phys=%x\n",
                   (uint32_t)(virt + i * PAGE_SIZE),
                   (uint32_t)paging_translate(
                       directory,
                       virt + i * PAGE_SIZE));
            printf("VMM: virtual page %x already mapped\n",
                   virt + i * PAGE_SIZE);
            return false;
        }
    }

    for (i = 0; i < count; i++)
    {
        void *phys;

        phys = pmm_alloc_page();

        if (phys == NULL)
        {
            printf("VMM: physical allocation failed after %u pages\n",
                   i);

            vmm_unmap_pages(virt, i);

            return false;
        }

        paging_map(
            directory,
            virt + i * PAGE_SIZE,
            (uintptr_t)phys,
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    return true;
}

// API Functions

void *vmm_alloc_page(uintptr_t virt)
{
    return vmm_alloc_pages(virt, 1);
}

void vmm_free_page(uintptr_t virt)
{
    vmm_free_pages(virt, 1);
}

void *vmm_alloc_pages(uintptr_t virt,
                      uint32_t count)
{
    if (count == 0)
    {
        printf("VMM: invalid page count\n");
        return NULL;
    }

    if (!vmm_map_pages(virt, count))
        return NULL;

    return (void *)virt;
}

void vmm_free_pages(uintptr_t virt,
                    uint32_t count)
{
    vmm_unmap_pages(virt, count);
}

/*
 * TODO:
 * Introduce a convenience API that releases both:
 *   - physical mappings (VMM)
 *   - virtual reservation (VAM)
 *
 * Current callers must invoke:
 *
 *     vmm_free_pages(...)
 *     vam_free_pages(...)
 */
void *vmm_alloc_pages_any(uint32_t count)
{
    uintptr_t virt;

    if (count == 0)
    {
        printf("VMM: invalid page count\n");
        return NULL;
    }

    virt = (uintptr_t)vam_alloc_pages(count);

    if (virt == 0)
    {
        printf("VMM: virtual allocation failed\n");
        return NULL;
    }

    if (!vmm_map_pages(virt, count))
    {
        vam_free_pages(virt, count);
        return NULL;
    }

    return (void *)virt;
}
