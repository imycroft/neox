#include "vam.h"

#include "memory.h"
#include "string.h"
#include "printf.h"

static uint8_t bitmap[VAM_BITMAP_SIZE];

// Private functions

static void bitmap_set(uint32_t index)
{
    bitmap[index / 8] |= (1u << (index % 8));
}

static void bitmap_clear(uint32_t index)
{
     bitmap[index / 8] &= ~(1u << (index % 8));
}

static bool bitmap_test(uint32_t index)
{
    return (bitmap[index / 8] & (1u << (index % 8))) != 0;
}

 static bool bitmap_range_free(uint32_t start,
                               uint32_t count)
 {
     uint32_t i;

     for (i = 0; i < count; i++)
     {
         if (bitmap_test(start + i))
             return false;
     }

     return true;
 }

 static void bitmap_range_set(uint32_t start,
                              uint32_t count)
 {
     uint32_t i;

     for (i = 0; i < count; i++)
     {
         bitmap_set(start + i);
     }
 }

 static void bitmap_range_clear(uint32_t start,
                                uint32_t count)
 {
     uint32_t i;

     for (i = 0; i < count; i++)
     {
         bitmap_clear(start + i);
     }
 }

// API Functions

void vam_init(void)
{
    uint32_t page;

    memset(bitmap, 0, sizeof(bitmap));

    for (page = 0;
         page < (VAM_START / PAGE_SIZE);
    page++)
         {
             bitmap_set(page);
         }
}

void *vam_alloc_pages(uint32_t count)
{
    uint32_t page;

    if (count == 0)
        return NULL;

    for (page = VAM_START / PAGE_SIZE;
         page <= TOTAL_VIRTUAL_PAGES - count;
    page++)
         {
             if (bitmap_range_free(page, count))
             {
                 bitmap_range_set(page, count);

                 return (void *)(uintptr_t)(page * PAGE_SIZE);
             }
         }

         return NULL;
}

void vam_free_pages(uintptr_t addr,
                    uint32_t count)
{
    uint32_t page;

    if (count == 0)
        return;

    page = addr / PAGE_SIZE;

    bitmap_range_clear(page, count);
}

// Debug functions

void vam_dump(void)
{
    uint32_t i;

    printf("==== VAM ====\n");

    for (i = 1020; i < 1028; i++)
    {
        printf("Page %u : %s\n",
               i,
               bitmap_test(i) ? "USED" : "FREE");
    }

    printf("=============\n");
}
