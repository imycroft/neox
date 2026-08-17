#include "heap.h"
#include "vmm.h"
#include "paging.h"
#include "printf.h"

#define HEAP_START KERNEL_HEAP_START
#define HEAP_PAGES 1

struct heap_block
{
    uint32_t size;
    bool free;
    struct heap_block *next;
}__attribute__((aligned(8)));

// Private Functions

static uint32_t heap_align(uint32_t size)
{
    return (size + HEAP_ALIGNMENT - 1)
    & ~(HEAP_ALIGNMENT - 1);
}

static struct heap_block *heap_head;
static uintptr_t heap_end;
static uint32_t heap_expansions;

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

static struct heap_block *heap_last_block(void)
{
    struct heap_block *block;

    block = heap_head;

    if (block == NULL)
        return NULL;

    while (block->next != NULL)
        block = block->next;

    return block;
}

static void heap_merge(void)
{
    struct heap_block *block;

    block = heap_head;

    while (block != NULL && block->next != NULL)
    {
        if (block->free && block->next->free)
        {
            block->size +=
            sizeof(struct heap_block) +
            block->next->size;

            block->next = block->next->next;
        }
        else
        {
            block = block->next;
        }
    }
}

static bool heap_expand(void)
{
    struct heap_block *block;

    if (vmm_alloc_pages(heap_end, 1) == NULL)
        return false;


    block = heap_last_block();

    if (block == NULL)
    {
        vmm_free_page(heap_end);
        return false;
    }

    block->next = (struct heap_block *)heap_end;

    block = block->next;

    block->size =
    PAGE_SIZE - sizeof(struct heap_block);

    block->free = true;
    block->next = NULL;

    heap_end += PAGE_SIZE;
    //
    heap_expansions++;
    //
    heap_merge();

    return true;
}


static void heap_split_block(struct heap_block *block,
                             uint32_t size)
{
    struct heap_block *new_block;

    if (block->size <=
        size + sizeof(struct heap_block))
    {
        return;
    }

    new_block = (struct heap_block *)
    ((uint8_t *)(block + 1) + size);

    new_block->size =
    block->size - size - sizeof(struct heap_block);

    new_block->free = true;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}



// API Functions

void heap_init(void)
{
    if (vmm_alloc_pages(KERNEL_HEAP_START, HEAP_PAGES) == NULL)
    {
        printf("Heap: allocation failed\n");
        return;
    }
    // for Debugging
    heap_expansions = 0;
    //
    heap_head = (struct heap_block *)KERNEL_HEAP_START;
    heap_end  = KERNEL_HEAP_START + HEAP_PAGES * PAGE_SIZE;

    printf("heap head %x\n", heap_head);
    printf("heap end %x\n", heap_end);
    printf("heap size before %x\n", heap_head->size );
    heap_head->size =
    HEAP_PAGES * PAGE_SIZE -
    sizeof(struct heap_block);

    printf("heap size %x\n", heap_head->size );

    heap_head->free = true;
    heap_head->next = NULL;

    printf("Heap initialized\n");
    printf("Heap start : %x\n", HEAP_START);
    printf("Heap size  : %u bytes\n",
           HEAP_PAGES * PAGE_SIZE);
}


void *kmalloc(uint32_t size)
{
    struct heap_block *block;

    if (size == 0)
        return NULL;

    size = heap_align(size);

    block = heap_find_free(size);

    while (block == NULL)
    {
        if (!heap_expand())
        {
            printf("kmalloc: out of memory\n");
            return NULL;
        }

        block = heap_find_free(size);
    }

    if (block->size >=
        size + sizeof(struct heap_block) + 1)
    {
        heap_split_block(block, size);
    }

    block->free = false;

    return (void *)(block + 1);
}

void kfree(void *ptr)
{
    struct heap_block *block;

    if (ptr == NULL)
        return;

    block = ((struct heap_block *)ptr) - 1;

    block->free = true;

    heap_merge();
}



// Debugging functions

void heap_dump(void)
{
    struct heap_block *block;
    uint32_t index;

    block = heap_head;
    index = 0;

    printf("\n=== HEAP DUMP ===\n");

    while (block != NULL)
    {
        printf("Block %u\n", index);
        printf("  Header : %x\n", (uint32_t)block);
        printf("  Data   : %x\n", (uint32_t)(block + 1));
        printf("  Size   : %u\n", block->size);
        printf("  Free   : %s\n",
               block->free ? "Yes" : "No");
        printf("  Next   : %x\n",
               (uint32_t)block->next);

        block = block->next;
        index++;
    }
    printf("Heap expansions: %u\n", heap_expansions);
    printf("Blocks : %u\n", index);
    printf("=================\n");
}

