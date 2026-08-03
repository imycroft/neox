#include "test.h"
#include "paging.h"
#include "memory.h"
#include "util.h"

#define PAGING_STRESS_PAGES 32768

/* ALL THE TEST FUNCTIONS GO HERE */

static void test_paging_map_one_page(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = 0x00400000;
    phys = 0x00200000;

    paging_map(
        virt,
        phys,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   phys
    );

    paging_unmap(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    test_pass();
}

static void test_paging_translate_offset(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = 0x00500000;
    phys = 0x00300000;

    paging_map(
        virt,
        phys,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(virt + 0x123),
                   phys + 0x123
    );

    TEST_ASSERT_EQ(
        paging_translate(virt + 0xFFF),
                   phys + 0xFFF
    );

    paging_unmap(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt + 0x123),
                   0
    );

    test_pass();
}

static void test_paging_remap_page(void)
{
    uintptr_t virt;
    uintptr_t phys1;
    uintptr_t phys2;

    virt  = 0x00600000;
    phys1 = 0x00200000;
    phys2 = 0x00300000;

    paging_map(
        virt,
        phys1,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   phys1
    );

    paging_map(
        virt,
        phys2,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   phys2
    );

    paging_unmap(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    test_pass();
}

static void test_paging_map_many_pages(void)
{
    uintptr_t virt;
    uintptr_t phys;
    uint32_t i;

    virt = 0x00800000;
    phys = 0x02000000;

    for (i = 0; i < 64; i++)
    {
        paging_map(
            virt + (i * PAGE_SIZE),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < 64; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * PAGE_SIZE)),
                       phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 64; i++)
    {
        paging_unmap(
            virt + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 64; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * PAGE_SIZE)),
                       0
        );
    }

    test_pass();
}

static void test_paging_directory_boundary(void)
{
    uintptr_t virt1;
    uintptr_t virt2;
    uintptr_t phys1;
    uintptr_t phys2;

    virt1 = 0x00BFF000;
    virt2 = 0x00C00000;

    phys1 = 0x03000000;
    phys2 = 0x03100000;

    paging_map(
        virt1,
        phys1,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    paging_map(
        virt2,
        phys2,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(virt1),
                   phys1
    );

    TEST_ASSERT_EQ(
        paging_translate(virt2),
                   phys2
    );

    paging_unmap(virt1);
    paging_unmap(virt2);

    TEST_ASSERT_EQ(
        paging_translate(virt1),
                   0
    );

    TEST_ASSERT_EQ(
        paging_translate(virt2),
                   0
    );

    test_pass();
}

static void test_paging_translate_unmapped(void)
{
    uintptr_t virt;

    virt = 0x02000000;

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    test_pass();
}

static void test_paging_unmap_unmapped(void)
{
    paging_unmap(0x02400000);

    TEST_ASSERT_EQ(
        paging_translate(0x02400000),
                   0
    );

    test_pass();
}

static void test_paging_unmap_one_does_not_affect_others(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = 0x01000000;
    phys = 0x04000000;

    paging_map(virt + (0 * PAGE_SIZE),
               phys + (0 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_map(virt + (1 * PAGE_SIZE),
               phys + (1 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_map(virt + (2 * PAGE_SIZE),
               phys + (2 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_unmap(virt + PAGE_SIZE);

    TEST_ASSERT_EQ(
        paging_translate(virt + (0 * PAGE_SIZE)),
                   phys + (0 * PAGE_SIZE)
    );

    TEST_ASSERT_EQ(
        paging_translate(virt + (1 * PAGE_SIZE)),
                   0
    );

    TEST_ASSERT_EQ(
        paging_translate(virt + (2 * PAGE_SIZE)),
                   phys + (2 * PAGE_SIZE)
    );

    paging_unmap(virt + (0 * PAGE_SIZE));
    paging_unmap(virt + (2 * PAGE_SIZE));

    test_pass();
}

static void test_paging_many_page_tables(void)
{
    uintptr_t virt;
    uintptr_t phys;
    uint32_t i;

    virt = 0x04000000;
    phys = 0x08000000;

    for (i = 0; i < 16; i++)
    {
        paging_map(
            virt + (i * 0x400000),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * 0x400000)),
                       phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 16; i++)
    {
        paging_unmap(
            virt + (i * 0x400000)
        );
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * 0x400000)),
                       0
        );
    }

    test_pass();
}

static void test_paging_map_thousands_of_pages(void)
{
    uintptr_t virt;
    uintptr_t phys;
    uint32_t i;

    virt = 0x10000000;
    phys = 0x20000000;

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        paging_map(
            virt + (i * PAGE_SIZE),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * PAGE_SIZE)),
                       phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        paging_unmap(
            virt + (i * PAGE_SIZE)
        );
    }


    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + (i * PAGE_SIZE)),
                       0
        );
    }

    test_pass();
}


/* ======== END OF TEST FUNCTIONS */


// Register the test functions
static test_entry_t tests[] =
{
    { "paging_map_one_page",  test_paging_map_one_page },
    { "paging_translate_offset", test_paging_translate_offset },
    { "paging_remap_page", test_paging_remap_page },
    { "paging_map_many_pages", test_paging_map_many_pages },
    { "paging_directory_boundary", test_paging_directory_boundary },
    { "paging_translate_unmapped", test_paging_translate_unmapped },
    { "paging_unmap_unmapped", test_paging_unmap_unmapped },
    { "paging_unmap_one_does_not_affect_others", test_paging_unmap_one_does_not_affect_others },
    { "paging_many_page_tables", test_paging_many_page_tables },
    {
        "paging_map_thousands_of_pages",
        test_paging_map_thousands_of_pages
    },
};

void test_paging(void)
{
    uint32_t i;

    test_begin("Paging");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);
        tests[i].func();
    }

    test_end();
}
