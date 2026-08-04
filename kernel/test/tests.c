#include "tests.h"
#include "test.h"

void kernel_tests(void)
{
       // test_memory();

       // TODO :
       // The test crashes when activated with the other tests, I will investigate why later;

       // test_paging();

//       test_vam();
//
//       test_vmm();
//
//       test_heap();
//       test_string();
//
//       test_process();
//
//       test_thread();
//
//       test_scheduler();

//      test_list();
       test_wait();

      test_summary();
}
