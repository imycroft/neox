#pragma once

#include "types.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

const struct multiboot_info *multiboot2_info(void);

struct multiboot_info
{
    uint32_t total_size;
    uint32_t reserved;
};

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry
{
    uint64_t addr;
    uint64_t len;

    uint32_t type;
    uint32_t reserved;
}__attribute__((packed));

struct multiboot_tag_mmap
{
    uint32_t type;
    uint32_t size;

    uint32_t entry_size;
    uint32_t entry_version;

    struct multiboot_mmap_entry entries[];
}__attribute__((packed));

struct multiboot_tag_module
{
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char     cmdline[];
} __attribute__((packed));

#define MULTIBOOT_TAG_TYPE_END          0
#define MULTIBOOT_TAG_TYPE_CMDLINE      1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER  2
#define MULTIBOOT_TAG_TYPE_MODULE       3
#define MULTIBOOT_TAG_TYPE_BASIC_MEM    4
#define MULTIBOOT_TAG_TYPE_MMAP         6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER  8

#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

void multiboot2_init(uint32_t magic,
                     struct multiboot_info *mb_info);

uint32_t multiboot2_total_size(void);

const struct multiboot_tag *
multiboot2_find_tag(uint32_t type);

const struct multiboot_tag_mmap *multiboot2_memory_map(void);

const struct multiboot_tag_module *
multiboot2_module(void);

// temporary debug functions

void multiboot2_dump_tags(void);
void multiboot2_dump_memory_map(void);
void multiboot2_dump_module(void);
