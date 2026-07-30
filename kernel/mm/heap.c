#include "heap.h"
#include "vmm.h"
#include "paging.h"
#include "printf.h"

#define HEAP_START 0x01000000
#define HEAP_PAGES 1

struct heap_block
{
    uint32_t size;
    bool free;
    struct heap_block *next;
};

static struct heap_block *heap_head;

static struct heap_block *heap_find_free(uint32_t size)
{
    struct heap_block *block;

    block = heap_head;

    while (block != NULL)
    {
        if (block->free && block->size >= size)
            return block;

        block = block->next;
    }

    return NULL;
}



void heap_init(void)
{
    if (vmm_alloc_pages(HEAP_START, HEAP_PAGES) == NULL)
    {
        printf("Heap: allocation failed\n");
        return;
    }

    heap_head = (struct heap_block *)HEAP_START;

    heap_head->size =
    HEAP_PAGES * PAGE_SIZE -
    sizeof(struct heap_block);

    heap_head->free = true;
    heap_head->next = NULL;

    printf("Heap initialized\n");
    printf("Heap start : %x\n", HEAP_START);
    printf("Heap size  : %u bytes\n",
           HEAP_PAGES * PAGE_SIZE);
}

static void heap_split_block(struct heap_block *block,
                             uint32_t size)
{
    struct heap_block *new_block;

    new_block = (struct heap_block *)
    ((uint8_t *)(block + 1) + size);

    new_block->size =
    block->size - size - sizeof(struct heap_block);

    new_block->free = true;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

void *kmalloc(uint32_t size)
{
    struct heap_block *block;

    if (size == 0)
        return NULL;

    block = heap_find_free(size);

    if (block == NULL)
    {
        printf("kmalloc: out of memory\n");
        return NULL;
    }

    if (block->size >=
        size + sizeof(struct heap_block) + 1)
    {
        heap_split_block(block, size);
    }

    block->free = false;

    return (void *)(block + 1);
}


