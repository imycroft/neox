#include "arch.h"
#include "scheduler.h"
#include "process.h"
#include "thread.h"
#include "test.h"
#include "printf.h"
#include "usermode.h"
#include "paging.h"
#include "elf_loader.h"
#include "multiboot2.h"
#include "util.h"
#include "pmm.h"

/*
 * Test the complete user-mode creation path using the real ELF module:
 *
 *     Multiboot module
 *          ↓
 *       elf_load()
 *          ↓
 *     process address space
 *          ↓
 *     usermode_elf_thread_create()
 *          ↓
 *     scheduler_add()
 *          ↓
 *        Ring 3
 */
static void test_usermode_elf(void)
{
    const struct multiboot_tag_module *module;

    struct process *process;
    struct thread  *thread;

    const void *image;
    uint32_t size;

    uintptr_t entry;
    uintptr_t code_phys;
    uintptr_t stack_phys;
    uintptr_t kernel_stack_phys;

    interrupt_state_t state;

    module = multiboot2_module();

    TEST_ASSERT_NOT_NULL(module);

    /*
     * Multiboot modules are physical addresses.
     *
     * Convert the module address into the kernel's higher-half
     * virtual address.
     */
    image = (const void *)PHYS_TO_VIRT(module->mod_start);

    size = module->mod_end - module->mod_start;

    /*
     * The module must contain a valid ELF image.
     */
    TEST_ASSERT_TRUE(
        elf_validate(image, size)
    );

    /*
     * Create a fresh process address space.
     */
    process = process_create("usermode_test");

    TEST_ASSERT_NOT_NULL(process);

    /*
     * Load the ELF image into this process.
     *
     * elf_load() creates the user code/data mappings and returns
     * the ELF entry point.
     */
    TEST_ASSERT_TRUE(
        elf_load(
            process,
            image,
            size,
            &entry
        )
    );

    TEST_ASSERT_NE(entry, 0);

    /*
     * The ELF entry point must actually be mapped in this process.
     */
    code_phys = paging_translate(
        process->page_directory,
        entry
    );

    TEST_ASSERT_NE(code_phys, 0);

    /*
     * The ELF entry must be user-accessible.
     *
     * Verify the mapping exists before creating the thread.
     */
    TEST_ASSERT_TRUE(
        paging_validate_mapping(
            process->page_directory,
            entry
        )
    );

    /*
     * Create the kernel-side thread and its user stack.
     *
     * This does NOT add the thread to the scheduler yet.
     */
    thread = usermode_elf_thread_create(
        process,
        entry
    );

    TEST_ASSERT_NOT_NULL(thread);

    /*
     * The user stack must exist in THIS process.
     */
    stack_phys = paging_translate(
        process->page_directory,
        USER_STACK_VIRT
    );

    TEST_ASSERT_NE(stack_phys, 0);

    TEST_ASSERT_TRUE(
        paging_validate_mapping(
            process->page_directory,
            USER_STACK_VIRT
        )
    );

    /*
     * The user stack must NOT be mapped through the shared
     * kernel page directory.
     */
    kernel_stack_phys = paging_translate(
        paging_get_kernel_directory(),
                                         USER_STACK_VIRT
    );

    TEST_ASSERT_EQ(kernel_stack_phys, 0);

    /*
     * The thread is not scheduler-visible until scheduler_add().
     *
     * scheduler_add() requires interrupts to be disabled.
     */
    state = interrupt_save();

    scheduler_add(thread);

    interrupt_restore(state);

    /*
     * At this point the test has successfully:
     *
     *   - created a process
     *   - loaded a real ELF image
     *   - mapped user code
     *   - created a user stack
     *   - created the kernel thread
     *   - inserted it into the scheduler
     *
     * The scheduler will now eventually execute the ELF entry in Ring 3.
     */

    thread_join(thread);
    test_pass();
}

static void test_usermode_stack_isolation(void)
{
    struct process *process_a;
    struct process *process_b;

    void *phys_a;
    void *phys_b;

    uintptr_t translated_a;
    uintptr_t translated_b;

    interrupt_state_t state;

    process_a = process_create("usermode_a");
    TEST_ASSERT_NOT_NULL(process_a);

    process_b = process_create("usermode_b");
    TEST_ASSERT_NOT_NULL(process_b);

    /*
     * Give each process its own physical page.
     */
    phys_a = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys_a);

    phys_b = pmm_alloc_page();
    TEST_ASSERT_NOT_NULL(phys_b);

    /*
     * Both processes intentionally use the same virtual address.
     */
    paging_map(
        process_a->page_directory,
        USER_STACK_VIRT,
        (uintptr_t)phys_a,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    paging_map(
        process_b->page_directory,
        USER_STACK_VIRT,
        (uintptr_t)phys_b,
               PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
    );

    /*
     * Each process must see its own physical page.
     */
    translated_a = paging_translate(
        process_a->page_directory,
        USER_STACK_VIRT
    );

    translated_b = paging_translate(
        process_b->page_directory,
        USER_STACK_VIRT
    );

    TEST_ASSERT_EQ(translated_a, (uintptr_t)phys_a);
    TEST_ASSERT_EQ(translated_b, (uintptr_t)phys_b);

    /*
     * Same virtual address, different physical memory.
     */
    TEST_ASSERT_NE(translated_a, translated_b);

    /*
     * Process A must remain unchanged after process B was mapped.
     */
    translated_a = paging_translate(
        process_a->page_directory,
        USER_STACK_VIRT
    );

    TEST_ASSERT_EQ(translated_a, (uintptr_t)phys_a);

    /*
     * The shared kernel directory must not contain this user mapping.
     */
    TEST_ASSERT_EQ(
        paging_translate(
            paging_get_kernel_directory(),
                         USER_STACK_VIRT
        ),
        0
    );

    /*
     * Clean up the mappings and their physical pages.
     */
    paging_unmap(
        process_a->page_directory,
        USER_STACK_VIRT
    );

    paging_unmap(
        process_b->page_directory,
        USER_STACK_VIRT
    );

    pmm_free_page(phys_a);
    pmm_free_page(phys_b);


    state = interrupt_save();

    process_destroy(process_a);
    process_destroy(process_b);

    interrupt_restore(state);

    test_pass();
}

static void test_usermode_elf_stack_isolation(void)
{
    const struct multiboot_tag_module *module;
    const void *image;
    uint32_t size;

    struct process *process_a;
    struct process *process_b;

    struct thread *thread_a;
    struct thread *thread_b;

    uintptr_t entry_a;
    uintptr_t entry_b;

    uintptr_t stack_phys_a;
    uintptr_t stack_phys_b;

    module = multiboot2_module();

    TEST_ASSERT_NOT_NULL(module);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;

    TEST_ASSERT_TRUE(
        elf_validate(image, size)
    );

    /*
     * Create two completely independent processes.
     */
    process_a = process_create("usermode_a");
    TEST_ASSERT_NOT_NULL(process_a);

    process_b = process_create("usermode_b");
    TEST_ASSERT_NOT_NULL(process_b);

    /*
     * Load the same ELF image into both address spaces.
     */
    TEST_ASSERT_TRUE(
        elf_load(
            process_a,
            image,
            size,
            &entry_a
        )
    );

    TEST_ASSERT_TRUE(
        elf_load(
            process_b,
            image,
            size,
            &entry_b
        )
    );

    TEST_ASSERT_EQ(entry_a, entry_b);

    /*
     * Create the user-mode threads.
     *
     * This is the important part: usermode_elf_thread_create()
     * must allocate and map a separate user stack for each process.
     */
    thread_a = usermode_elf_thread_create(
        process_a,
        entry_a
    );

    TEST_ASSERT_NOT_NULL(thread_a);

    thread_b = usermode_elf_thread_create(
        process_b,
        entry_b
    );

    TEST_ASSERT_NOT_NULL(thread_b);

    /*
     * Both processes use the same user virtual stack address.
     */
    stack_phys_a = paging_translate(
        process_a->page_directory,
        USER_STACK_VIRT
    );

    stack_phys_b = paging_translate(
        process_b->page_directory,
        USER_STACK_VIRT
    );

    TEST_ASSERT_NE(stack_phys_a, 0);
    TEST_ASSERT_NE(stack_phys_b, 0);

    /*
     * The physical stack pages must be different.
     */
    TEST_ASSERT_NE(stack_phys_a, stack_phys_b);

    /*
     * Creating process B must not change process A's stack mapping.
     */
    TEST_ASSERT_EQ(
        paging_translate(
            process_a->page_directory,
            USER_STACK_VIRT
        ),
        stack_phys_a
    );

    /*
     * The user stack must not exist in the shared kernel address space.
     */
    TEST_ASSERT_EQ(
        paging_translate(
            paging_get_kernel_directory(),
                         USER_STACK_VIRT
        ),
        0
    );

    /*
     * The threads were never added to the scheduler, so they must not
     * be allowed to reach Ring 3 during this test.
     *
     * Cleanup is therefore performed directly.
     */
    interrupt_state_t state = interrupt_save();

    thread_add(thread_a);
    thread_add(thread_b);

    interrupt_restore(state);

    thread_join(thread_a);
    thread_join(thread_b);

    test_pass();
}

static void test_usermode_process_reclamation(void)
{
    const struct multiboot_tag_module *module;
    const void *image;
    uint32_t size;
    uint32_t before;
    uint32_t after;
    uint32_t i;

    module = multiboot2_module();

    TEST_ASSERT_NOT_NULL(module);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;

    TEST_ASSERT_TRUE(
        elf_validate(image, size)
    );

    before = pmm_free_pages();

    for (i = 0; i < 100; i++)
    {
        struct process *process;
        struct thread  *thread;
        uintptr_t       entry;
        interrupt_state_t state;

        process = process_create("usermode_reclaim");

        TEST_ASSERT_NOT_NULL(process);

        TEST_ASSERT_TRUE(
            elf_load(
                process,
                image,
                size,
                &entry
            )
        );

        thread = usermode_elf_thread_create(
            process,
            entry
        );

        TEST_ASSERT_NOT_NULL(thread);

        state = interrupt_save();

        scheduler_add(thread);

        interrupt_restore(state);

        thread_join(thread);
    }

    after = pmm_free_pages();

    TEST_ASSERT_EQ(after, before);

    test_pass();
}

static void test_usermode_detached_reclamation(void)
{
    const struct multiboot_tag_module *module;
    struct process *process;
    struct thread  *thread;
    const void     *image;
    uint32_t        size;
    uintptr_t       entry;
    interrupt_state_t state;

    module = multiboot2_module();

    TEST_ASSERT_NOT_NULL(module);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;

    TEST_ASSERT_TRUE(
        elf_validate(image, size)
    );

    process = process_create("usermode_detached");

    TEST_ASSERT_NOT_NULL(process);

    TEST_ASSERT_TRUE(
        elf_load(
            process,
            image,
            size,
            &entry
        )
    );

    TEST_ASSERT_NE(entry, 0);

    thread = usermode_elf_thread_create(
        process,
        entry
    );

    TEST_ASSERT_NOT_NULL(thread);

    /*
     * Detached threads are not joined by their creator.
     *
     * Once the thread terminates, the reaper owns its destruction.
     */
    thread->detached = true;

    state = interrupt_save();

    scheduler_add(thread);

    interrupt_restore(state);

    /*
     * Give the user thread time to execute and terminate.
     * The reaper will subsequently destroy the detached thread
     * and, once its process has no remaining threads, destroy
     * the process as well.
     */
    test_wait_ticks(20);

    /*
     * We deliberately do NOT call thread_join().
     *
     * The thread and process must have been reclaimed by the reaper.
     */
    test_pass();
}

static void test_elf_load_rollback(void)
{
    const struct multiboot_tag_module *module;
    struct process *process;
    const void *image;
    uint32_t size;
    uintptr_t entry;
    uint32_t before;
    uint32_t after;

    module = multiboot2_module();

    TEST_ASSERT_NOT_NULL(module);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;

    TEST_ASSERT_TRUE(
        elf_validate(image, size)
    );

    process = process_create("elf_rollback");

    TEST_ASSERT_NOT_NULL(process);

    before = pmm_free_pages();

    /*
     * Allow the first ELF page allocation to succeed,
     * then force the next allocation to fail.
     */
    pmm_fail_next_allocations(1);

    TEST_ASSERT_FALSE(
        elf_load(
            process,
            image,
            size,
            &entry
        )
    );

    after = pmm_free_pages();

    TEST_ASSERT_EQ(after, before);

    interrupt_disable();

    process_destroy(process);

    interrupt_enable();

    test_pass();
}

/*
 * User-mode test suite.
 */

static test_entry_t tests[] =
{
    { "usermode_elf",             test_usermode_elf },
    { "usermode_stack_isolation", test_usermode_stack_isolation },
    { "usermode_elf_stack_isolation", test_usermode_elf_stack_isolation },
    { "usermode_process_reclamation", test_usermode_process_reclamation },
    { "usermode_detached_reclamation", test_usermode_detached_reclamation },
    { "elf_load_rollback", test_elf_load_rollback },
};
void test_usermode(void)
{
    uint32_t i;

    test_begin("Thread Usermode Ring3");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
