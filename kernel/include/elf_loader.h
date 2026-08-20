#pragma once

#include "types.h"

/*
 * An ELF image is loaded into a page directory.
 *
 * The ELF loader does not need to know about the process itself.
 * It only needs the address space into which the ELF segments
 * must be mapped.
 */
struct page_directory;

/*
 * Validate the ELF header and program-header table.
 */
int elf_validate(
    const void *image,
    uint32_t size
);

/*
 * Print information about all PT_LOAD segments.
 *
 * This is a debugging helper.
 */
void elf_dump_load_segments(
    const void *image
);

/*
 * Load an ELF executable into the specified address space.
 *
 * @directory : page directory receiving the ELF mappings
 * @image     : complete ELF file image
 * @size      : size of the ELF image
 * @entry     : receives the ELF entry-point virtual address
 *
 * Returns:
 *     1 on success
 *     0 on failure
 */
int elf_load(
    struct page_directory *directory,
    const void *image,
    uint32_t size,
    uintptr_t *entry
);
