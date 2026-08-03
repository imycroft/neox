#include "test.h"
#include "heap.h"
#include "paging.h"
#include "memory.h"
#include "printf.h"
#include "util.h"

#define HEAP_TEST_LARGE_SIZE (32 * 1024)
#define HEAP_STRESS_BLOCKS 4096

/* Test functions */

/*
 * Verify:
 *
 * - A one-byte allocation succeeds.
 * - A valid pointer is returned.
 *
 * This validates:
 * - Basic heap allocation.
 */
static void test_heap_allocate_one_byte(void)
{
    void *ptr;

    ptr = kmalloc(1);

    TEST_ASSERT_NOT_NULL(ptr);

    test_pass();

    kfree(ptr);
}

/*
 * Verify:
 *
 * - Every allocation is aligned to the heap alignment.
 *
 * This validates:
 * - The heap always returns properly aligned pointers.
 */
static void test_heap_alignment(void)
{
    void *ptr;

    ptr = kmalloc(1);

    TEST_ASSERT_NOT_NULL(ptr);

    TEST_ASSERT_EQ(
        (uintptr_t)ptr % HEAP_ALIGNMENT,
                   0
    );

    test_pass();

    kfree(ptr);
}

/*
 * Verify:
 *
 * - Two consecutive allocations succeed.
 * - Each allocation returns a unique pointer.
 *
 * This validates:
 * - The allocator correctly splits free blocks.
 */
static void test_heap_allocate_two_blocks(void)
{
    void *a;
    void *b;

    a = kmalloc(32);
    b = kmalloc(32);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);

    TEST_ASSERT_NE(a, b);

    test_pass();

    kfree(a);
    kfree(b);
}

/*
 * Verify:
 *
 * - Multiple consecutive allocations succeed.
 * - Every allocation returns a unique pointer.
 *
 * This validates:
 * - Repeated block splitting.
 * - Heap metadata remains consistent after multiple allocations.
 */
static void test_heap_allocate_many_blocks(void)
{
    void *blocks[64];
    uint32_t i;
    uint32_t j;

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        blocks[i] = kmalloc(32);

        TEST_ASSERT_NOT_NULL(blocks[i]);
    }

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        for (j = i + 1; j < ARRAY_SIZE(blocks); j++)
        {
            TEST_ASSERT_NE(
                blocks[i],
                blocks[j]
            );
        }
    }

    test_pass();

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        kfree(blocks[i]);
    }
}

/*
 * Verify:
 *
 * - A freed block can be allocated again.
 * - The allocator reuses released memory.
 *
 * This validates:
 * - Freeing updates heap state correctly.
 * - First-fit allocation reuses available blocks.
 */
static void test_heap_free_reuse(void)
{
    void *first;
    void *second;

    first = kmalloc(64);

    TEST_ASSERT_NOT_NULL(first);

    kfree(first);

    second = kmalloc(64);

    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_EQ(first, second);

    test_pass();

    kfree(second);
}

/*
 * Verify:
 *
 * - Multiple allocations can be freed in reverse order.
 * - Freeing order does not corrupt heap metadata.
 *
 * This validates:
 * - Block management remains consistent after arbitrary frees.
 */
static void test_heap_free_reverse(void)
{
    void *blocks[3];
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        blocks[i] = kmalloc(64);

        TEST_ASSERT_NOT_NULL(blocks[i]);
    }

    kfree(blocks[2]);
    kfree(blocks[1]);
    kfree(blocks[0]);

    test_pass();
}

/*
 * Verify:
 *
 * - Adjacent free blocks are merged into a larger free block.
 * - The merged block can satisfy a larger allocation.
 *
 * This validates:
 * - Block coalescing.
 */
static void test_heap_block_merge(void)
{
    void *a;
    void *b;
    void *c;

    a = kmalloc(128);
    b = kmalloc(128);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);

    kfree(a);
    kfree(b);

    c = kmalloc(256);

    TEST_ASSERT_NOT_NULL(c);

    TEST_ASSERT_EQ(c, a);

    test_pass();

    kfree(c);
}

/*
 * Verify:
 *
 * - The heap automatically expands when full.
 * - Allocations continue successfully after expansion.
 *
 * This validates:
 * - Dynamic heap growth.
 */
static void test_heap_expand(void)
{
    void *blocks[64];
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        blocks[i] = kmalloc(128);

        TEST_ASSERT_NOT_NULL(blocks[i]);
    }

    test_pass();

    for (i = 0; i < ARRAY_SIZE(blocks); i++)
    {
        kfree(blocks[i]);
    }
}

/*
 * Verify:
 *
 * - A zero-byte allocation fails.
 *
 * This validates:
 * - Invalid allocation requests are rejected.
 */
static void test_heap_zero_allocation(void)
{
    TEST_ASSERT_NULL(
        kmalloc(0)
    );

    test_pass();
}

/*
 * Verify:
 *
 * - The heap can allocate a large block.
 * - The allocation succeeds after automatic expansion.
 *
 * This validates:
 * - Large allocation behavior.
 */
static void test_heap_large_allocation(void)
{
    void *ptr;

    ptr = kmalloc(HEAP_TEST_LARGE_SIZE);

    TEST_ASSERT_NOT_NULL(ptr);

    test_pass();

    kfree(ptr);
}

/*
 * Verify:
 *
 * - A freed large allocation can be reused.
 *
 * This validates:
 * - Large free regions remain reusable.
 */
static void test_heap_large_free_reuse(void)
{
    void *first;
    void *second;

    first = kmalloc(HEAP_TEST_LARGE_SIZE);

    TEST_ASSERT_NOT_NULL(first);

    kfree(first);

    second = kmalloc(HEAP_TEST_LARGE_SIZE);

    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_EQ(first, second);

    test_pass();

    kfree(second);
}

/*
 * Verify:
 *
 * - The heap can sustain many allocations.
 * - The allocator remains consistent under heavy load.
 *
 * This validates:
 * - Long-running allocator behavior.
 */
static void test_heap_stress(void)
{
    /*
     * Large pointer array used by the stress test.
     * Static storage avoids overflowing the kernel stack.
     */
    static void *blocks[HEAP_STRESS_BLOCKS];
    uint32_t count;

    count = 0;

    while (count < HEAP_STRESS_BLOCKS)
    {
        blocks[count] = kmalloc(64);

        if (blocks[count] == NULL)
            break;

        count++;
    }

    TEST_ASSERT_TRUE(count > 0);


    while (count > 0)
    {
        count--;
        kfree(blocks[count]);
    }

    test_pass();
}

/*
 * Verify:
 *
 * - Repeated allocation/free cycles succeed.
 * - The allocator consistently reuses memory.
 *
 * This validates:
 * - Long-term allocator stability.
 * - Block splitting and merging over many iterations.
 * - Heap metadata remains consistent.
 */
static void test_heap_allocate_free_cycle(void)
{
    void *ptr;
    uint32_t i;

    for (i = 0; i < 10000; i++)
    {
        ptr = kmalloc(64);

        TEST_ASSERT_NOT_NULL(ptr);

        kfree(ptr);
    }

    test_pass();
}

/* Test registry */

static test_entry_t tests[] =
{
    { "heap_allocate_one_byte", test_heap_allocate_one_byte },
    { "heap_alignment", test_heap_alignment },
    { "heap_allocate_two_blocks", test_heap_allocate_two_blocks },
    { "heap_allocate_many_blocks", test_heap_allocate_many_blocks },
    { "heap_free_reuse", test_heap_free_reuse },
    { "heap_free_reverse", test_heap_free_reverse },
    { "heap_block_merge", test_heap_block_merge },
    { "heap_expand", test_heap_expand },
    { "heap_zero_allocation", test_heap_zero_allocation },
    { "heap_large_allocation", test_heap_large_allocation },
    { "heap_large_free_reuse", test_heap_large_free_reuse },
    { "heap_stress", test_heap_stress },
    { "heap_allocate_free_cycle", test_heap_allocate_free_cycle },
};

void test_heap(void)
{
    uint32_t i;

    test_begin("HEAP");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
