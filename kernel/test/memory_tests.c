#include "test.h"
#include "pmm.h"
#include "memory.h"
#include "printf.h"

#define PMM_TEST_PAGES 64
#define PMM_MAX_TEST_PAGES 70000

// Definitions


// Private functions

static void free_pages(void **pages, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
        pmm_free_page(pages[i]);
}

static void test_pmm_allocate_one_page(void)
{
    void *page;

    page = pmm_alloc_page();

    TEST_ASSERT_NOT_NULL(page);

    test_pass();

    pmm_free_page(page);

}

static void test_pmm_allocate_two_pages(void)
{
    void *page1;
    void *page2;

    page1 = pmm_alloc_page();
    page2 = pmm_alloc_page();

    TEST_ASSERT_NOT_NULL(page1);
    TEST_ASSERT_NOT_NULL(page2);

    TEST_ASSERT_NE(page1,page2);

    test_pass();

    pmm_free_page(page1);
    pmm_free_page(page2);
}

/*
 * Current PMM policy:
 * Always returns the lowest available physical page.
 */
static void test_pmm_allocate_three_pages(void)
{
    void *a;
    void *b;
    void *c;

    a = pmm_alloc_page();
    b = pmm_alloc_page();
    c = pmm_alloc_page();

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    TEST_ASSERT_EQ(
        (uintptr_t)b,
                   (uintptr_t)a + PAGE_SIZE
    );

    TEST_ASSERT_EQ(
        (uintptr_t)c,
                   (uintptr_t)b + PAGE_SIZE
    );

    test_pass();

    pmm_free_page(a);
    pmm_free_page(b);
    pmm_free_page(c);
}

static void test_pmm_allocate_many_pages(void)
{
    void *pages[PMM_TEST_PAGES];
    uint32_t i;

    for (i = 0; i < PMM_TEST_PAGES; i++)
    {
        pages[i] = pmm_alloc_page();

        TEST_ASSERT_NOT_NULL(pages[i]);
    }

    free_pages(pages, PMM_TEST_PAGES);

    test_pass();
}

static void test_pmm_unique_pages(void)
{
    void *pages[PMM_TEST_PAGES];
    uint32_t i;
    uint32_t j;

    for (i = 0; i < PMM_TEST_PAGES; i++)
    {
        pages[i] = pmm_alloc_page();

        TEST_ASSERT_NOT_NULL(pages[i]);
    }

    for (i = 0; i < PMM_TEST_PAGES; i++)
    {
        for (j = i + 1; j < PMM_TEST_PAGES; j++)
        {
            TEST_ASSERT_NE(pages[i], pages[j]);
        }
    }

    free_pages(pages, PMM_TEST_PAGES);

    test_pass();
}

static void test_pmm_page_alignment(void)
{
    void *page;

    page = pmm_alloc_page();

    TEST_ASSERT_NOT_NULL(page);

    TEST_ASSERT_EQ(
        (uintptr_t)page % PAGE_SIZE,
                   0
    );

    test_pass();

    pmm_free_page(page);
}

static void test_pmm_many_pages_alignment(void)
{
    void *pages[PMM_TEST_PAGES];
    uint32_t i;

    for (i = 0; i < PMM_TEST_PAGES; i++)
    {
        pages[i] = pmm_alloc_page();

        TEST_ASSERT_NOT_NULL(pages[i]);

        TEST_ASSERT_EQ(
            (uintptr_t)pages[i] % PAGE_SIZE,
                       0
        );
    }

    free_pages(pages, PMM_TEST_PAGES);

    test_pass();
}

static void test_pmm_free_reuse(void)
{
    void *page1;
    void *page2;

    page1 = pmm_alloc_page();

    pmm_free_page(page1);

    page2 = pmm_alloc_page();

    TEST_ASSERT_EQ(page1, page2);

    test_pass();

    pmm_free_page(page2);
}

static void test_pmm_free_reverse(void)
{
    void *pages[PMM_TEST_PAGES];
    int32_t i;

    for (i = 0; i < PMM_TEST_PAGES; i++)
    {
        pages[i] = pmm_alloc_page();

        TEST_ASSERT_NOT_NULL(pages[i]);
    }

    free_pages(pages, PMM_TEST_PAGES);

    test_pass();
}

static void test_pmm_reuse_lowest_page(void)
{
    void *a;
    void *b;
    void *c;
    void *d;

    a = pmm_alloc_page();
    b = pmm_alloc_page();
    c = pmm_alloc_page();

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    pmm_free_page(b);

    d = pmm_alloc_page();

    TEST_ASSERT_EQ(d, b);

    pmm_free_page(a);
    pmm_free_page(c);
    pmm_free_page(d);

    test_pass();
}

static void test_pmm_allocate_until_oom(void)
{
    static void *pages[PMM_MAX_TEST_PAGES];
    uint32_t count;

    test_case("pmm_allocate_until_oom");

    count = 0;

    while (count < PMM_MAX_TEST_PAGES)
    {
        pages[count] = pmm_alloc_page();

        if (pages[count] == NULL)
            break;

        count++;

        if ((count % 1000) == 0)
        {
            printf("%u\n", count);
        }
    }

    TEST_ASSERT_TRUE(count > 0);

    printf("Allocated %u pages\n", count);

    free_pages(pages, count);

    test_pass();
}

static void test_pmm_recover_after_oom(void)
{
    void *pages[PMM_MAX_TEST_PAGES];
    uint32_t first_count;
    uint32_t second_count;

    test_case("pmm_recover_after_oom");

    first_count = 0;

    while (first_count < PMM_MAX_TEST_PAGES)
    {
        pages[first_count] = pmm_alloc_page();

        if (pages[first_count] == NULL)
            break;

        first_count++;
    }

    free_pages(pages, first_count);

    second_count = 0;

    while (second_count < PMM_MAX_TEST_PAGES)
    {
        pages[second_count] = pmm_alloc_page();

        if (pages[second_count] == NULL)
            break;

        second_count++;
    }

    TEST_ASSERT_EQ(first_count, second_count);

    free_pages(pages, second_count);

    test_pass();
}
// API

static test_entry_t tests[] =
{
    { "pmm_allocate_one_page",       test_pmm_allocate_one_page       },
    { "pmm_allocate_two_pages",      test_pmm_allocate_two_pages      },
    { "pmm_allocate_three_pages",    test_pmm_allocate_three_pages    },
    { "pmm_allocate_many_pages",     test_pmm_allocate_many_pages     },
    { "pmm_unique_pages",            test_pmm_unique_pages            },
    { "pmm_page_alignment",          test_pmm_page_alignment          },
    { "pmm_many_pages_alignment",    test_pmm_many_pages_alignment    },
    { "pmm_free_reuse",              test_pmm_free_reuse              },
    { "pmm_free_reverse",            test_pmm_free_reverse            },
    { "pmm_reuse_lowest_page",       test_pmm_reuse_lowest_page       },
    { "pmm_allocate_until_oom",      test_pmm_allocate_until_oom      },
    { "pmm_recover_after_oom",       test_pmm_recover_after_oom       },
};

void test_memory(void)
{
    uint32_t i;

    test_begin("PMM");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
