#ifndef TESTS_H
#define TESTS_H

void kernel_tests(void);

void test_memory(void);
void test_paging(void);
void test_heap(void);
void test_vam(void);
void test_vmm(void);

void test_string(void);
void test_process(void);
void test_thread(void);
void test_scheduler(void);

void test_thread_integration(void);
void test_thread_preemption(void);

void test_list(void);
void test_wait(void);

void test_semaphore(void);
void test_mutex(void);

#endif
