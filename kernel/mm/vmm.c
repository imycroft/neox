#include "vmm.h"
#include "paging.h"
#include "pmm.h"
#include "vam.h"
#include "printf.h"

// Private functions
static bool vmm_map_pages(uintptr_t virt,
                          uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        void *phys;

        phys = pmm_alloc_page();

        if (phys == NULL)
        {
            printf("VMM: physical allocation failed\n");

            /*
             * TODO:
             * Roll back:
             *   - unmap mapped pages
             *   - free physical pages
             */

            return false;
        }

        paging_map(
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
    if (!vmm_map_pages(virt, count))
        return NULL;

    return (void *)virt;
}

void vmm_free_pages(uintptr_t virt, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        uintptr_t phys;

        phys = paging_translate(virt + i * PAGE_SIZE);

        if (phys != 0)
        {
            paging_unmap(virt + i * PAGE_SIZE);

            pmm_free_page(
                (void *)(phys & ~(PAGE_SIZE - 1))
            );
        }
    }
}

void *vmm_alloc_pages_any(uint32_t count)
{
    uintptr_t virt;

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
