#include "tests.h"
#include "test.h"

void kernel_tests(void)
{
    test_memory();

    /* later */
    // test_heap();
    // test_vam();
    // test_vmm();

    test_summary();
}
