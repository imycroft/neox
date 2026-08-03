#include "test.h"
#include "vam.h"
#include "memory.h"
#include "printf.h"
#include "util.h"

#define VAM_TEST_PAGES 32768


/* Test functions */

static void test_vam_allocate_one_page(void)
{
    void *addr;

    addr = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(addr);

    test_pass();

    vam_free_pages((uintptr_t)addr, 1);
}

/*
 * Verify that every allocated virtual address is aligned to a page boundary.
 * We're not testing where the allocation starts—that will be a separate test.
 */
static void test_vam_page_alignment(void)
{
    void *addr;

    addr = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_EQ(
        (uintptr_t)addr % PAGE_SIZE,
                   0
    );

    test_pass();

    vam_free_pages((uintptr_t)addr, 1);
}

/*
 * Verify:
 *
 * Two-page allocation succeeds.
 * The allocator returns a valid page-aligned address.
 * The allocation can be released correctly.
 *
 * This validates basic multi-page allocation.
 */
static void test_vam_allocate_two_pages(void)
{
    void *addr;

    addr = vam_alloc_pages(2);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_EQ(
        (uintptr_t)addr % PAGE_SIZE,
                   0
    );

    test_pass();

    vam_free_pages((uintptr_t)addr, 2);
}

/*
 * Verify:
 *
 * The allocator can reserve a larger virtual range.
 * The returned address remains page aligned.
 * The allocation can be freed correctly.
 */
static void test_vam_allocate_many_pages(void)
{
    void *addr;

    addr = vam_alloc_pages(64);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_EQ(
        (uintptr_t)addr % PAGE_SIZE,
                   0
    );

    test_pass();

    vam_free_pages((uintptr_t)addr,64);
}

/*
 * Verify that two active allocations never return the same virtual address.
 *
 * This validates that allocated virtual ranges do not overlap.
 */
static void test_vam_unique_addresses(void)
{
    void *first;
    void *second;

    first = vam_alloc_pages(1);
    second = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_NE(first, second);

    test_pass();

    vam_free_pages((uintptr_t)first, 1);
    vam_free_pages((uintptr_t)second, 1);
}

/*
 * Verify:
 *
 * - A freed virtual page can be allocated again.
 * - The allocator reuses released virtual space.
 *
 * This validates:
 * - Freeing correctly updates allocator state.
 */
static void test_vam_free_reuse(void)
{
    void *first;
    void *second;

    first = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(first);

    vam_free_pages((uintptr_t)first, 1);

    second = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_EQ(first, second);

    test_pass();

    vam_free_pages((uintptr_t)second, 1);
}

/*
 * Verify:
 *
 * - Multiple allocations can be freed in reverse order.
 * - Freeing order does not corrupt allocator state.
 *
 * This validates:
 * - Bitmap state remains consistent after multiple frees.
 */
static void test_vam_free_reverse(void)
{
    void *pages[3];
    uint32_t i;

    for (i = 0; i < 3; i++)
    {
        pages[i] = vam_alloc_pages(1);

        TEST_ASSERT_NOT_NULL(pages[i]);
    }

    vam_free_pages((uintptr_t)pages[2], 1);
    vam_free_pages((uintptr_t)pages[1], 1);
    vam_free_pages((uintptr_t)pages[0], 1);

    test_pass();
}

/*
 * Verify:
 *
 * - The allocator reuses the lowest available virtual page.
 *
 * This validates:
 * - The allocator follows first-fit allocation behavior.
 */
static void test_vam_reuse_lowest_page(void)
{
    void *a;
    void *b;
    void *c;
    void *d;

    a = vam_alloc_pages(1);
    b = vam_alloc_pages(1);
    c = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    vam_free_pages((uintptr_t)b, 1);

    d = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(d);

    TEST_ASSERT_EQ(d, b);

    test_pass();

    vam_free_pages((uintptr_t)a, 1);
    vam_free_pages((uintptr_t)c, 1);
    vam_free_pages((uintptr_t)d, 1);
}

/*
 * Verify:
 *
 * - Allocated virtual addresses are always inside the VAM range.
 *
 * This validates:
 * - Kernel reserved virtual space is never returned by the allocator.
 */
static void test_vam_kernel_space_reserved(void)
{
    void *addr;

    addr = vam_alloc_pages(1);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_TRUE(
        (uintptr_t)addr >= VAM_START
    );

    test_pass();

    vam_free_pages((uintptr_t)addr, 1);
}
/*
 * Verify:
 *
 * - The allocator can allocate many virtual pages.
 * - Allocation stops correctly when the requested limit is reached.
 *
 * This validates:
 * - Large range allocation behavior.
 */
// THIS TEST TAKES TOO MUCH TIME
/*static void test_vam_allocate_many_pages_stress(void)
{
    uint32_t count;
    void *addr;

    count = 0;

    while (count < VAM_TEST_MAX_PAGES)
    {
        if (count % 1000 == 0) printf("%d\n", count);

        addr = vam_alloc_pages(1);

        TEST_ASSERT_NOT_NULL(addr);

        count++;
    }

    printf("Allocated %u virtual pages\n", count);

    test_pass();
}
*/
/*
 * Verify:
 *
 * - The allocator can reserve a large virtual range.
 * - The allocated range can be released.
 *
 * This validates:
 * - Large contiguous virtual allocation.
 */
static void test_vam_large_allocation(void)
{
    void *addr;

    addr = vam_alloc_pages(VAM_TEST_PAGES);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_EQ(
        (uintptr_t)addr % PAGE_SIZE,
                   0
    );

    test_pass();

    vam_free_pages(
        (uintptr_t)addr,
                   VAM_TEST_PAGES
    );
}

/*
 * Verify:
 *
 * - A large allocation can be freed.
 * - The released range becomes available again.
 *
 * This validates bitmap cleanup after large allocations.
 */
static void test_vam_large_free_reuse(void)
{
    void *first;
    void *second;

    first = vam_alloc_pages(VAM_TEST_PAGES);

    TEST_ASSERT_NOT_NULL(first);

    vam_free_pages(
        (uintptr_t)first,
                   VAM_TEST_PAGES
    );

    second = vam_alloc_pages(VAM_TEST_PAGES);

    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_EQ(first, second);

    test_pass();

    vam_free_pages(
        (uintptr_t)second,
                   VAM_TEST_PAGES
    );
}

/* ======== END OF TEST FUNCTIONS */


/* Test registry */

static test_entry_t tests[] =
{
    { "vam_allocate_one_page", test_vam_allocate_one_page },
    { "vam_page_alignment",    test_vam_page_alignment    },
    { "vam_allocate_two_pages", test_vam_allocate_two_pages },
    { "vam_allocate_many_pages", test_vam_allocate_many_pages },
    { "vam_unique_addresses",    test_vam_unique_addresses    },
    { "vam_free_reuse",          test_vam_free_reuse          },
    { "vam_free_reverse",        test_vam_free_reverse        },
    { "vam_reuse_lowest_page",    test_vam_reuse_lowest_page     },
    { "vam_kernel_space_reserved",       test_vam_kernel_space_reserved      },
    /*{
        "vam_allocate_many_pages_stress",
        test_vam_allocate_many_pages_stress
    },*/
    {"vam_large_allocation", test_vam_large_allocation},
    {"vam_large_free_reuse", test_vam_large_free_reuse},

};

void test_vam(void)
{
    uint32_t i;

    test_begin("VAM");

    vam_init();

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
