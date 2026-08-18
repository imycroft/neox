#include "arch.h"
#include "test.h"
#include "paging.h"
#include "memory.h"
#include "util.h"
#include "printf.h"
#include "pmm.h"
#define PAGING_STRESS_PAGES 32768

/* ALL THE TEST FUNCTIONS GO HERE */

static void test_paging_map_one_page(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = 0x00400000;
    phys = 0x00200000;

    paging_map(
        paging_get_kernel_directory(),
        virt,
        phys,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
                   phys
    );

    paging_unmap(paging_get_kernel_directory(), virt);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
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
        paging_get_kernel_directory(),
        virt,
        phys,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt + 0x123),
                   phys + 0x123
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt + 0xFFF),
                   phys + 0xFFF
    );

    paging_unmap(paging_get_kernel_directory(), virt);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt + 0x123),
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
        paging_get_kernel_directory(),
        virt,
        phys1,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
                   phys1
    );

    paging_map(
        paging_get_kernel_directory(),
        virt,
        phys2,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
                   phys2
    );

    paging_unmap(paging_get_kernel_directory(), virt);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
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
            paging_get_kernel_directory(),
            virt + (i * PAGE_SIZE),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < 64; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(),
                             virt + (i * PAGE_SIZE)),
                             phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 64; i++)
    {
        paging_unmap(
            paging_get_kernel_directory(),
            virt + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 64; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(),
                             virt + (i * PAGE_SIZE)),
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
        paging_get_kernel_directory(),
        virt1,
        phys1,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    paging_map(
        paging_get_kernel_directory(),
        virt2,
        phys2,
        PAGE_PRESENT | PAGE_WRITABLE
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt1),
                   phys1
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt2),
                   phys2
    );

    paging_unmap(paging_get_kernel_directory(), virt1);
    paging_unmap(paging_get_kernel_directory(), virt2);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt1),
                   0
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt2),
                   0
    );

    test_pass();
}

static void test_paging_translate_unmapped(void)
{
    uintptr_t virt;

    virt = 0x02000000;

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), virt),
                   0
    );

    test_pass();
}

static void test_paging_unmap_unmapped(void)
{
    paging_unmap(paging_get_kernel_directory(), 0x02400000);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(), 0x02400000),
                   0
    );

    test_pass();
}

static void test_paging_unmap_one_does_not_affect_others(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = 0x00800000;
    phys = 0x04000000;

    paging_map(paging_get_kernel_directory(),
               virt + (0 * PAGE_SIZE),
               phys + (0 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_map(paging_get_kernel_directory(),
               virt + (1 * PAGE_SIZE),
               phys + (1 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_map(paging_get_kernel_directory(),
               virt + (2 * PAGE_SIZE),
               phys + (2 * PAGE_SIZE),
               PAGE_PRESENT | PAGE_WRITABLE);

    paging_unmap(paging_get_kernel_directory(),
                 virt + PAGE_SIZE);

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(),
                         virt + (0 * PAGE_SIZE)),
                   phys + (0 * PAGE_SIZE)
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(),
                         virt + (1 * PAGE_SIZE)),
                   0
    );

    TEST_ASSERT_EQ(
        paging_translate(paging_get_kernel_directory(),
                         virt + (2 * PAGE_SIZE)),
                   phys + (2 * PAGE_SIZE)
    );

    paging_unmap(paging_get_kernel_directory(), virt + (0 * PAGE_SIZE));
    paging_unmap(paging_get_kernel_directory(), virt + (2 * PAGE_SIZE));

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
        paging_map(paging_get_kernel_directory(),
                   virt + (i * 0x400000),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(),
                             virt + (i * 0x400000)),
                             phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < 16; i++)
    {
        paging_unmap(paging_get_kernel_directory(),
                     virt + (i * 0x400000)
        );
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(), virt + (i * 0x400000)),
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
        paging_map(paging_get_kernel_directory(),
                virt + (i * PAGE_SIZE),
                   phys + (i * PAGE_SIZE),
                   PAGE_PRESENT | PAGE_WRITABLE
        );
    }

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(), virt + (i * PAGE_SIZE)),
                       phys + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        paging_unmap(paging_get_kernel_directory(),
                     virt + (i * PAGE_SIZE)
        );
    }

    for (i = 0; i < PAGING_STRESS_PAGES; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(paging_get_kernel_directory(), virt + (i * PAGE_SIZE)),
                       0
        );
    }

    test_pass();
}

static void test_paging_kernel_mappings_copied(void)
{
    struct page_directory *kernel_directory;
    struct page_directory *directory;
    uint32_t i;

    kernel_directory = paging_get_kernel_directory();

    directory = paging_create_directory();

    TEST_ASSERT_NE(directory, NULL);

    for (i = KERNEL_PDE_START;
         i < PAGE_DIRECTORY_ENTRIES;
    i++)
         {
             TEST_ASSERT_EQ(
                 directory->entries[i],
                 kernel_directory->entries[i]
             );
         }

         pmm_free_page(directory);

         test_pass();
}

static void test_paging_destroy_directory_reclaims(void)
{
    uint32_t before;
    uint32_t after;
    struct page_directory *directory;
    void *phys;

    before = pmm_free_pages();

    directory = paging_create_directory();
    TEST_ASSERT_NOT_NULL(directory);

    phys = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys);

    paging_map(
        directory,
        0x00400000,
        (uintptr_t)phys,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    TEST_ASSERT_NE(
        paging_translate(directory, 0x00400000),
                   0
    );

    interrupt_disable();

    paging_destroy_directory(directory);

    interrupt_enable();

    after = pmm_free_pages();

    TEST_ASSERT_EQ(after, before);

    test_pass();
}

static void test_paging_unmap_reclaims_empty_table(void)
{
    struct page_directory *directory;
    void *phys;
    uint32_t before;
    uint32_t after;

    before = pmm_free_pages();

    directory = paging_create_directory();
    TEST_ASSERT_NOT_NULL(directory);

    phys = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys);

    paging_map(
        directory,
        0x00400000,
        (uintptr_t)phys,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    TEST_ASSERT_NE(
        paging_translate(directory, 0x00400000),
                   0
    );

    /*
     * Unmap the only page in the page table.
     *
     * paging_unmap() should therefore reclaim the page table.
     */
    paging_unmap(
        directory,
        0x00400000
    );

    TEST_ASSERT_EQ(
        paging_translate(directory, 0x00400000),
                   0
    );

    /*
     * The physical page is owned by the caller, not paging_unmap().
     */
    pmm_free_page(phys);

    /*
     * The directory itself still exists.
     */
    interrupt_disable();

    paging_destroy_directory(directory);

    interrupt_enable();

    after = pmm_free_pages();

    TEST_ASSERT_EQ(after, before);

    test_pass();
}

static void test_paging_unmap_keeps_nonempty_table(void)
{
    struct page_directory *directory;
    void *phys_a;
    void *phys_b;
    uint32_t before;
    uint32_t after;

    before = pmm_free_pages();

    directory = paging_create_directory();
    TEST_ASSERT_NOT_NULL(directory);

    phys_a = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys_a);

    phys_b = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys_b);

    /*
     * Both pages use the same page table.
     */
    paging_map(
        directory,
        0x00400000,
        (uintptr_t)phys_a,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    paging_map(
        directory,
        0x00401000,
        (uintptr_t)phys_b,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    TEST_ASSERT_NE(
        paging_translate(directory, 0x00400000),
                   0
    );

    TEST_ASSERT_NE(
        paging_translate(directory, 0x00401000),
                   0
    );

    /*
     * Remove the first mapping.
     *
     * The page table must NOT be reclaimed because the second
     * mapping is still present.
     */
    paging_unmap(
        directory,
        0x00400000
    );

    TEST_ASSERT_EQ(
        paging_translate(directory, 0x00400000),
                   0
    );

    TEST_ASSERT_NE(
        paging_translate(directory, 0x00401000),
                   0
    );

    /*
     * Remove the second mapping.
     *
     * The page table is now empty and must be reclaimed.
     */
    paging_unmap(
        directory,
        0x00401000
    );

    TEST_ASSERT_EQ(
        paging_translate(directory, 0x00401000),
                   0
    );

    /*
     * paging_unmap() does not own the physical pages.
     */
    pmm_free_page(phys_a);
    pmm_free_page(phys_b);

    interrupt_disable();

    paging_destroy_directory(directory);

    interrupt_enable();

    after = pmm_free_pages();

    TEST_ASSERT_EQ(after, before);

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
    { "paging_map_thousands_of_pages",test_paging_map_thousands_of_pages},
    { "paging_kernel_mappings_copied", test_paging_kernel_mappings_copied },
    { "paging_destroy_directory_reclaims", test_paging_destroy_directory_reclaims },
    { "paging_unmap_reclaims_empty_table", test_paging_unmap_reclaims_empty_table },
    { "paging_unmap_keeps_nonempty_table", test_paging_unmap_keeps_nonempty_table },
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
