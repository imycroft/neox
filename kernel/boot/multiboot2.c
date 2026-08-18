#include "multiboot2.h"
#include "arch.h"
#include "memory_layout.h"
#include "panic.h"
#include "printf.h"
#include "paging.h"
#include "elf_loader.h"
#include "process.h"

#include "scheduler.h"
#include "usermode.h"

static struct multiboot_info *boot_info;


static const struct multiboot_tag *multiboot2_first_tag(void);

static const struct multiboot_tag *multiboot2_next_tag(
    const struct multiboot_tag *tag);


void multiboot2_init(uint32_t magic,
                     struct multiboot_info *mb_info)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        panic("Invalid Multiboot2 magic");

    /*
     * GRUB passes mb_info as a physical address.
     *
     * Before the higher-half transition this was used directly through
     * the identity mapping.  Now that paging is active and the kernel
     * runs in the higher half, low physical addresses below 0xC0000000
     * are not guaranteed to be mapped.
     *
     * The bootstrap maps the first 16 MiB of physical memory at both:
     *
     *     0x00000000  (identity)
     *     0xC0000000  (higher-half)
     *
     * GRUB places the multiboot info structure in low memory (typically
     * around 0x00007000), well within the first 16 MiB.
     *
     * PHYS_TO_VIRT() converts it to its higher-half virtual address,
     * which is valid under the current page directory.
     */
    boot_info = (struct multiboot_info *)
    PHYS_TO_VIRT((uintptr_t)mb_info);
}

const struct multiboot_info *multiboot2_info(void)
{
    return boot_info;
}

uint32_t multiboot2_total_size(void)
{
    return boot_info->total_size;
}

const struct multiboot_tag *multiboot2_first_tag(void)
{
    return (const struct multiboot_tag *)
    ((uint8_t *)boot_info + 8);
}

const struct multiboot_tag *
multiboot2_next_tag(const struct multiboot_tag *tag)
{
    uint32_t size;

    size = (tag->size + 7) & ~7u;

    return (const struct multiboot_tag *)
    ((const uint8_t *)tag + size);
}

const struct multiboot_tag *
multiboot2_find_tag(uint32_t type)
{
    const struct multiboot_tag *tag;

    tag = multiboot2_first_tag();

    while (tag->type != MULTIBOOT_TAG_TYPE_END)
    {
        if (tag->type == type)
            return tag;

        tag = multiboot2_next_tag(tag);
    }

    return NULL;
}

const struct multiboot_tag_mmap *multiboot2_memory_map(void)
{
    return (const struct multiboot_tag_mmap *)
    multiboot2_find_tag(MULTIBOOT_TAG_TYPE_MMAP);
}

const struct multiboot_tag_module *
multiboot2_module(void)
{
    return (const struct multiboot_tag_module *)
    multiboot2_find_tag(MULTIBOOT_TAG_TYPE_MODULE);
}

// for debug purposes

void multiboot2_dump_memory_map(void)
{
    const struct multiboot_tag_mmap *mmap;
    const struct multiboot_mmap_entry *entry;
    uint32_t count;
    uint32_t i;

    mmap = multiboot2_memory_map();

    if (mmap == NULL)
    {
        printf("Memory map not found\n");
        return;
    }

    count = (mmap->size - sizeof(struct multiboot_tag_mmap))
    / mmap->entry_size;

    printf("Memory map:\n");

    entry = mmap->entries;

    for (i = 0; i < count; i++)
    {
        printf("type=%u\n", entry->type);

        entry = (const struct multiboot_mmap_entry *)
        ((const uint8_t *)entry + mmap->entry_size);
    }
}

void multiboot2_dump_tags(void)
{
    const struct multiboot_tag *tag;

    tag = multiboot2_first_tag();

    while (1)
    {
        printf("Tag %u (size %u)\n",
               tag->type,
               tag->size);

        if (tag->type == MULTIBOOT_TAG_TYPE_END)
            break;

        tag = multiboot2_next_tag(tag);
    }
}

void multiboot2_dump_module(void)
{
    const struct multiboot_tag_module *module;
    const void *image;
    uint32_t size;

    module = multiboot2_module();

    if (module == NULL)
    {
        printf("Multiboot: no module found\n");
        return;
    }

    printf("Multiboot module:\n");
    printf("  start = %x\n", module->mod_start);
    printf("  end   = %x\n", module->mod_end);
    printf("  size  = %u\n", module->mod_end - module->mod_start);
    printf("  name  = %s\n", module->cmdline);

    image = (const void *)PHYS_TO_VIRT(module->mod_start);
    size = module->mod_end - module->mod_start;


    const uint8_t *bytes;

    uintptr_t translated;

    translated = paging_translate(
        paging_get_kernel_directory(),
                                  (uintptr_t)image
    );

    printf("ELF virtual  = %x\n", (uint32_t)image);
    printf("ELF physical = %x\n", (uint32_t)translated);

    bytes = (const uint8_t *)PHYS_TO_VIRT(module->mod_start);

    printf("ELF address = %x\n", (uint32_t)bytes);

    printf("ELF validation: %s\n",
            elf_validate(image, size) ? "OK" : "FAILED");

    if (elf_validate(image, size))
        elf_dump_load_segments(image);



    struct process *init_process;
    uintptr_t init_entry;

    init_process = process_create("init");


    if (init_process == NULL)
    {
        printf("ELF: failed to create init process\n");
        return;
    }

    if (!elf_load(init_process,
        image,
        size,
        &init_entry))
    {
        printf("ELF: failed to load init\n");
        return;
    }

    ///

    printf("ELF entry = %x\n", (uint32_t)init_entry);

    printf("loaded bytes:\n");

    uint32_t phys;

    for (uint32_t i = 0; i < 16; i++)
    {
        phys = paging_translate(
            init_process->page_directory,
            init_entry + i
        );

        printf("%x ", *(uint8_t *)PHYS_TO_VIRT(phys));
    }

    printf("\n");

    ///

    struct thread *init_thread;

    printf("ELF loaded: entry=%x\n", (uint32_t)init_entry);

    init_thread = usermode_elf_thread_create(
        init_process,
        init_entry
    );

    if (init_thread == NULL)
    {
        printf("ELF: failed to create user thread\n");
        return;
    }

    interrupt_disable();

    scheduler_add(init_thread);

    interrupt_enable();
}
