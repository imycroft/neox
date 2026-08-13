#include "tests.h"
#include "test.h"
#include "printf.h"

void kernel_tests(void)
{
      printf("KERNEL_TESTS: A\n");

      test_paging();

      printf("KERNEL_TESTS: B\n");

}
