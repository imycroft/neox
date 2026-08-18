#include "init.h"

#include "assert.h"
#include "elf_loader.h"
#include "multiboot2.h"
#include "panic.h"
#include "process.h"
#include "thread.h"
#include "usermode.h"

void init_process_start(void)
{
    const struct multiboot_tag_module *module;
    const void *image;
    uint32_t size;

    struct process *process;
    struct thread *thread;
    uintptr_t entry;

    module = multiboot2_module();

    ASSERT(module != NULL);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;

    ASSERT(elf_validate(image, size));

    process = process_create("init");

    ASSERT(process != NULL);

    if (!elf_load(process, image, size, &entry))
        panic("failed to load init");

    thread = usermode_elf_thread_create(
        process,
        entry
    );

    ASSERT(thread != NULL);

    thread_add(thread);
}
