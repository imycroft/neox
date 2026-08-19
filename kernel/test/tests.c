#include "tests.h"
#include "test.h"

#include "printf.h"

void kernel_tests(void)
{
        test_memory();
         test_paging();
        test_vam();
        test_vmm();
        test_heap();
        test_string();
        test_process();
        test_thread();
        test_scheduler();
        test_thread_integration();
        test_thread_preemption();
        test_list();
        test_wait();
        test_semaphore();
        test_mutex();
        test_condvar();

           test_usermode();

        test_summary();
}
