#include "test.h"
#include "paging.h"
#include "vam.h"
#include "vmm.h"
#include "memory.h"
#include "printf.h"
#include "util.h"

#define VMM_TEST_PAGES 1024
/* Test functions */

/*
 * Verify:
 *
 * - The VMM can allocate a page at a specified virtual address.
 * - The allocation succeeds.
 * - The virtual address becomes mapped.
 *
 * This validates:
 * - Basic VMM allocation.
 */
static void test_vmm_allocate_one_page(void)
{
    uintptr_t virt;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    TEST_ASSERT_NE(
        paging_translate(virt),
                   0
    );

    test_pass();

    vmm_free_page(virt);
}

/*
 * Verify:
 *
 * - A newly allocated virtual page has a valid physical mapping.
 * - The physical address is page aligned.
 *
 * This validates:
 * - Paging entries created by the VMM are valid.
 */
static void test_vmm_page_is_mapped(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    phys = paging_translate(virt);

    TEST_ASSERT_NE(phys, 0);

    TEST_ASSERT_EQ(
        phys % PAGE_SIZE,
        0
    );

    test_pass();

    vmm_free_page(virt);
}

/*
 * Verify:
 *
 * - The VMM can allocate multiple pages at a specified virtual address.
 * - Every virtual page becomes mapped.
 * - Every physical page is page aligned.
 *
 * This validates:
 * - Basic multi-page allocation.
 */
static void test_vmm_allocate_two_pages(void)
{
    uintptr_t virt;
    uintptr_t phys1;
    uintptr_t phys2;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, 2)
    );

    phys1 = paging_translate(virt);
    phys2 = paging_translate(virt + PAGE_SIZE);

    TEST_ASSERT_NE(phys1, 0);
    TEST_ASSERT_NE(phys2, 0);

    TEST_ASSERT_EQ(
        phys1 % PAGE_SIZE,
        0
    );

    TEST_ASSERT_EQ(
        phys2 % PAGE_SIZE,
        0
    );

    TEST_ASSERT_NE(
        phys1,
        phys2
    );

    test_pass();

    vmm_free_pages(virt, 2);
}

/*
 * Verify:
 *
 * - The VMM can allocate a larger contiguous virtual range.
 * - Every requested page becomes mapped.
 * - Every mapped physical page is page aligned.
 *
 * This validates:
 * - Multi-page allocation over a larger range.
 */
static void test_vmm_allocate_many_pages(void)
{
    uintptr_t virt;
    uintptr_t phys;
    uint32_t i;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, VMM_TEST_PAGES)
    );

    for (i = 0; i < VMM_TEST_PAGES; i++)
    {
        phys = paging_translate(
            virt + i * PAGE_SIZE
        );

        TEST_ASSERT_NE(
            phys,
            0
        );

        TEST_ASSERT_EQ(
            phys % PAGE_SIZE,
            0
        );
    }

    test_pass();

    vmm_free_pages(virt, VMM_TEST_PAGES);
}

/*
 * Verify:
 *
 * - Two separate virtual pages receive different physical pages.
 *
 * This validates:
 * - The VMM never maps two newly allocated pages to the same
 *   physical page.
 */
static void test_vmm_unique_physical_pages(void)
{
    uintptr_t virt;
    uintptr_t phys1;
    uintptr_t phys2;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, 2)
    );

    phys1 = paging_translate(virt);
    phys2 = paging_translate(virt + PAGE_SIZE);

    TEST_ASSERT_NE(
        phys1,
        phys2
    );

    test_pass();

    vmm_free_pages(virt, 2);
}

/*
 * Verify:
 *
 * - A freed virtual page can be allocated again.
 * - The page becomes mapped after reallocation.
 *
 * This validates:
 * - The VMM correctly releases mappings and physical pages.
 */
static void test_vmm_free_reuse(void)
{
    uintptr_t virt;
    uintptr_t phys1;
    uintptr_t phys2;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    phys1 = paging_translate(virt);

    TEST_ASSERT_NE(phys1, 0);

    vmm_free_page(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    phys2 = paging_translate(virt);

    TEST_ASSERT_NE(phys2, 0);

    TEST_ASSERT_EQ(
        phys1,
        phys2
    );

    test_pass();

    vmm_free_page(virt);
}

/*
 * Verify:
 *
 * - Multiple virtual pages can be freed in reverse order.
 * - Every mapping is removed.
 *
 * This validates:
 * - Free order does not affect VMM correctness.
 */
static void test_vmm_free_reverse(void)
{
    uintptr_t virt;
    uint32_t i;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, 3)
    );

    vmm_free_page(virt + 2 * PAGE_SIZE);
    vmm_free_page(virt + PAGE_SIZE);
    vmm_free_page(virt);

    for (i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQ(
            paging_translate(virt + i * PAGE_SIZE),
                       0
        );
    }

    test_pass();
}

/*
 * Verify:
 *
 * - A virtual page can be allocated again after being freed.
 * - The new allocation creates a valid mapping.
 *
 * This validates:
 * - The VMM completely removes mappings during free.
 */
static void test_vmm_reallocate_same_virtual_page(void)
{
    uintptr_t virt;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    vmm_free_page(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    TEST_ASSERT_NE(
        paging_translate(virt),
                   0
    );

    test_pass();

    vmm_free_page(virt);
}

/*
 * Verify:
 *
 * - The VMM can allocate pages without specifying a virtual address.
 * - A valid virtual address is returned.
 * - The returned address is page aligned.
 * - The returned virtual page is mapped.
 *
 * This validates:
 * - Integration between VAM, PMM, and Paging.
 */
static void test_vmm_alloc_pages_any(void)
{
    void *addr;

    addr = vmm_alloc_pages_any(1);

    TEST_ASSERT_NOT_NULL(addr);

    TEST_ASSERT_EQ(
        (uintptr_t)addr % PAGE_SIZE,
                   0
    );

    TEST_ASSERT_NE(
        paging_translate((uintptr_t)addr),
                   0
    );

    test_pass();

    vmm_free_pages((uintptr_t)addr, 1);
    vam_free_pages((uintptr_t)addr, 1);
}

/*
 * Verify:
 *
 * - Allocating over an already mapped virtual page fails.
 * - The original mapping remains unchanged.
 *
 * This validates:
 * - The VMM never overwrites existing mappings.
 */
static void test_vmm_alloc_over_existing_fails(void)
{
    uintptr_t virt;
    uintptr_t phys;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_page(virt)
    );

    phys = paging_translate(virt);

    TEST_ASSERT_NE(phys, 0);

    TEST_ASSERT_NULL(
        vmm_alloc_page(virt)
    );

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   phys
    );

    test_pass();

    vmm_free_page(virt);
}

/*
 * Verify:
 *
 * - Freeing an unmapped virtual page succeeds.
 * - No mappings are created or modified.
 *
 * This validates:
 * - vmm_free_page() is idempotent for unmapped pages.
 */
static void test_vmm_free_unmapped(void)
{
    uintptr_t virt;

    virt = VAM_START;

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    vmm_free_page(virt);

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    test_pass();
}

/*
 * Verify:
 *
 * - Allocating zero pages fails.
 * - No mapping is created.
 *
 * This validates:
 * - The VMM rejects invalid allocation requests.
 */
static void test_vmm_zero_page_allocation(void)
{
    uintptr_t virt;

    virt = VAM_START;

    TEST_ASSERT_NULL(
        vmm_alloc_pages(virt, 0)
    );

    TEST_ASSERT_EQ(
        paging_translate(virt),
                   0
    );

    test_pass();
}

/*
 * Verify:
 *
 * - Allocating zero pages with automatic virtual address selection fails.
 * - No virtual address is returned.
 *
 * This validates:
 * - Input validation for vmm_alloc_pages_any().
 */
static void test_vmm_alloc_pages_any_zero(void)
{
    TEST_ASSERT_NULL(
        vmm_alloc_pages_any(0)
    );

    test_pass();
}

/*
 * Verify:
 *
 * - The VMM can allocate a large contiguous virtual range.
 * - Every page in the range is mapped.
 *
 * This validates:
 * - Large multi-page allocation.
 */
static void test_vmm_large_allocation(void)
{
    uintptr_t virt;
    uintptr_t phys;
    uint32_t i;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, VMM_TEST_PAGES)
    );

    for (i = 0; i < VMM_TEST_PAGES; i++)
    {
        phys = paging_translate(
            virt + i * PAGE_SIZE
        );

        TEST_ASSERT_NE(
            phys,
            0
        );
    }

    test_pass();

    vmm_free_pages(virt, VMM_TEST_PAGES);
}

/*
 * Verify:
 *
 * - A large virtual allocation can be freed.
 * - The same virtual range can be allocated again.
 *
 * This validates:
 * - Large allocations are completely released.
 * - No mappings or physical pages are leaked.
 */
static void test_vmm_large_free_reuse(void)
{
    uintptr_t virt;

    virt = VAM_START;

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, VMM_TEST_PAGES)
    );

    vmm_free_pages(virt, VMM_TEST_PAGES);

    TEST_ASSERT_NOT_NULL(
        vmm_alloc_pages(virt, VMM_TEST_PAGES)
    );

    test_pass();

    vmm_free_pages(virt, VMM_TEST_PAGES);
}

/* Test registry */

static test_entry_t tests[] =
{
    { "vmm_allocate_one_page", test_vmm_allocate_one_page },
    { "vmm_page_is_mapped", test_vmm_page_is_mapped },
    { "vmm_allocate_two_pages", test_vmm_allocate_two_pages },
    { "vmm_allocate_many_pages", test_vmm_allocate_many_pages },
    { "vmm_unique_physical_pages", test_vmm_unique_physical_pages },
    { "vmm_free_reuse", test_vmm_free_reuse },
    { "vmm_free_reverse", test_vmm_free_reverse },
    { "vmm_reallocate_same_virtual_page", test_vmm_reallocate_same_virtual_page },
    { "vmm_alloc_pages_any", test_vmm_alloc_pages_any },
    { "vmm_alloc_over_existing_fails", test_vmm_alloc_over_existing_fails },
    { "vmm_free_unmapped", test_vmm_free_unmapped },
    { "vmm_zero_page_allocation", test_vmm_zero_page_allocation },
    { "vmm_alloc_pages_any_zero", test_vmm_alloc_pages_any_zero },
    { "vmm_large_allocation", test_vmm_large_allocation },
    { "vmm_large_free_reuse", test_vmm_large_free_reuse },

};

void test_vmm(void)
{
    uint32_t i;

    test_begin("VMM");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}

