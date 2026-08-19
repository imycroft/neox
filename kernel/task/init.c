#include "init.h"

#include "assert.h"
#include "elf_loader.h"
#include "file.h"
#include "fs.h"
#include "multiboot2.h"
#include "panic.h"
#include "process.h"
#include "thread.h"
#include "usermode.h"

void init_process_start(void)
{
    const struct multiboot_tag_module *module;
    const void *image;
    uint32_t image_size;

    struct process *process;
    struct thread *thread;
    uintptr_t entry;


    struct file file;

    /*
     * Locate the root filesystem supplied by the bootloader.
     */
    module = multiboot2_find_module("rootfs");

    ASSERT(module != NULL);

    /*
     * Convert the physical Multiboot module address into
     * the kernel's higher-half virtual address.
     */
    image = (const void *)PHYS_TO_VIRT(module->mod_start);

    image_size = module->mod_end - module->mod_start;

    /*
     * Mount the filesystem.
     */
    ASSERT(fs_mount(image, image_size) == 0);

    /*
     * Open the init executable through the normal file layer.
     */
    ASSERT(file_open(
        "/sbin/init",
        FILE_ACCESS_READ,
        &file
    ) == 0);

    /*
     * Obtain the ELF image from the opened file.
     */
    const void *file_image = file_data(&file);
    uint32_t file_size_value = file_size(&file);

    ASSERT(file_image != NULL);
    ASSERT(file_size_value != 0);

    /*
     * Verify that /sbin/init is a valid ELF executable.
     */
    ASSERT(elf_validate(
        file_image,
        file_size_value
    ));

    /*
     * Create the process that will contain the ELF image.
     */
    process = process_create("init");

    ASSERT(process != NULL);

    /*
     * Load the ELF segments into the new process address space.
     */
    if (!elf_load(
        process,
        file_image,
        file_size_value,
        &entry
    ))
    {
        panic("failed to load init");
    }

    /*
     * Create a kernel-side thread which will transition
     * into Ring 3 at the ELF entry point.
     */
    thread = usermode_elf_thread_create(
        process,
        entry
    );

    ASSERT(thread != NULL);

    /*
     * Make the init thread runnable.
     */
    thread_add(thread);

    /*
     * The file abstraction currently does not own any dynamically
     * allocated resources, so closing it simply invalidates the
     * descriptor.
     */
    file_close(&file);
}
