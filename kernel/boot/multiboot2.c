#include "multiboot2.h"

#include "panic.h"

#include "printf.h"

static struct multiboot_info *boot_info;


static const struct multiboot_tag *multiboot2_first_tag(void);

static const struct multiboot_tag *multiboot2_next_tag(
        const struct multiboot_tag *tag);


void multiboot2_init(uint32_t magic,
                     struct multiboot_info *mb_info)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
        panic("Invalid Multiboot2 magic");

    boot_info = mb_info;
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

    size = (tag->size + 7) & ~7;

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
