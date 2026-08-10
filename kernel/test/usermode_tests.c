#include "arch.h"
#include "scheduler.h"
#include "scheduler_internal.h"
#include "process.h"
#include "thread.h"
#include "test.h"
#include "printf.h"
#include "usermode.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* User-mode functions                                                  */
/*                                                                      */
/* These run in Ring 3.  They must only use INT 0x80 to communicate    */
/* with the kernel — no direct kernel calls, no kernel pointers.       */
/* ------------------------------------------------------------------ */

/*
 * User-mode entry: prints a message via SYS_WRITE then exits via SYS_EXIT.
 *
 * The string is a string literal that the linker places in the kernel
 * image.  Because we use a flat identity map with PAGE_USER on the user
 * stack page, kernel read-only text is also readable from Ring 3 in this
 * simple setup.  A real OS would copy the string into the user address
 * space, but for a smoke-test this is sufficient.
 */
static void user_hello(void)
{
    static const char msg[] = "[Ring 3] Hello from user mode!\n";
    const uint32_t    len   = sizeof(msg) - 1;

    /* SYS_WRITE(fd=1, buf, len) — we ignore fd in our handler */
    __asm__ volatile (
        "mov $4,  %%eax\n\t"   /* SYS_WRITE */
        "mov %0,  %%ebx\n\t"   /* buf       */
        "mov %1,  %%ecx\n\t"   /* len       */
        "xor %%edx, %%edx\n\t"
        "int $0x80\n\t"
        :
        : "r"(msg), "r"(len)
        : "eax", "ebx", "ecx", "edx"
    );

    /* SYS_EXIT(0) */
    __asm__ volatile (
        "mov $1,  %%eax\n\t"   /* SYS_EXIT */
        "xor %%ebx, %%ebx\n\t" /* code = 0 */
        "int $0x80\n\t"
        ::: "eax", "ebx"
    );

    /* Never reached */
    for (;;)
        __asm__ volatile ("hlt");
}

/*
 * Second user task: counts to 3 via SYS_WRITE then exits.
 */
static void user_counter(void)
{
    static const char *msgs[3] =
    {
        "[Ring 3] count 1\n",
        "[Ring 3] count 2\n",
        "[Ring 3] count 3\n",
    };

    uint32_t i;

    for (i = 0; i < 3; i++)
    {
        const char *s = msgs[i];
        uint32_t    n = 18; /* all strings are 18 chars */

        __asm__ volatile (
            "mov $4,  %%eax\n\t"
            "mov %0,  %%ebx\n\t"
            "mov %1,  %%ecx\n\t"
            "int $0x80\n\t"
            :
            : "r"(s), "r"(n)
            : "eax", "ebx", "ecx"
        );
    }

    __asm__ volatile (
        "mov $1,  %%eax\n\t"
        "xor %%ebx, %%ebx\n\t"
        "int $0x80\n\t"
        ::: "eax", "ebx"
    );

    for (;;)
        __asm__ volatile ("hlt");
}

/* ------------------------------------------------------------------ */
/* Kernel-side test harness                                             */
/* ------------------------------------------------------------------ */

/*
 * Wait for a user-mode thread to finish.
 * The thread is detached (the reaper cleans it up); we simply spin-
 * yield until the process has no more threads, or until a tick
 * budget expires.
 */
static void wait_for_user_thread(struct thread *t)
{
    /*
     * Join is the clean approach: it blocks until the thread
     * terminates then frees it.  It works for both kernel and
     * user-mode threads since SYS_EXIT ultimately calls
     * scheduler_terminate().
     */
    thread_join(t);
}

/*
 * test_ring3_hello — spawn one Ring-3 thread and wait for it.
 */
static void test_ring3_hello(void)
{
    struct process *process;
    struct thread  *thread;
    interrupt_state_t state;

    process = process_create("ring3_hello");
    TEST_ASSERT_NOT_NULL(process);

    thread = usermode_thread_create(process, user_hello);

    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();
    thread_add(thread);
    interrupt_restore(state);

    wait_for_user_thread(thread);
    printf("wait for user thread\n");
    test_pass();
}

/*
 * test_ring3_counter — spawn one Ring-3 thread that loops.
 */
static void test_ring3_counter(void)
{
    struct process *process;
    struct thread  *thread;
    interrupt_state_t state;

    process = process_create("ring3_counter");
    TEST_ASSERT_NOT_NULL(process);

    thread = usermode_thread_create(process, user_counter);
    TEST_ASSERT_NOT_NULL(thread);

    state = interrupt_save();
    thread_add(thread);
    interrupt_restore(state);

    wait_for_user_thread(thread);

    test_pass();
}

/* ------------------------------------------------------------------ */
/* Suite entry point                                                    */
/* ------------------------------------------------------------------ */

void test_usermode(void)
{
    printf("\n--- Ring 3 / User Mode Tests ---\n");

    test_ring3_hello();
    test_ring3_counter();

    printf("--- Ring 3 tests done ---\n\n");
}
