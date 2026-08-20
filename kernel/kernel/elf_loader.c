#include "elf_loader.h"
#include "elf.h"
#include "heap.h"
#include "process.h"
#include "pmm.h"
#include "paging.h"
#include "memory.h"
#include "string.h"
#include "list.h"
#include "util.h"
#include "printf.h"

struct elf_loaded_page
{
    uintptr_t    virt;
    struct list_node node;
};

int elf_validate(const void *image, uint32_t size)
{
    const struct elf32_header *header;

    if (image == NULL)
    {
        printf("image size null");
        return 0;
    }


    if (size < sizeof(struct elf32_header))
    {
        printf("size < sizeof(struct elf32_header)");
        return 0;
    }


    header = (const struct elf32_header *)image;

    if (header->e_ident[0] != ELF_MAGIC0 ||
        header->e_ident[1] != ELF_MAGIC1 ||
        header->e_ident[2] != ELF_MAGIC2 ||
        header->e_ident[3] != ELF_MAGIC3)

        {
            return 0;
        }


    if (header->e_ident[4] != ELFCLASS32)
    {
        return 0;
    }

    if (header->e_ident[5] != ELFDATA2LSB)
    {
        return 0;
    }

    if (header->e_type != ET_EXEC)
    {
        return 0;
    }

    if (header->e_machine != EM_386)
    {
        return 0;
    }

    if (header->e_phentsize != sizeof(struct elf32_program_header))
        return 0;

    if (header->e_phnum == 0)
        return 0;

    if (header->e_phoff > size)
        return 0;

    if ((uint32_t)header->e_phnum *
        sizeof(struct elf32_program_header) >
        size - header->e_phoff)
        return 0;

    return 1;
}

int elf_load(
    struct page_directory *directory,
    const void *image,
    uint32_t size,
    uintptr_t *entry
)
{
    const struct elf32_header *header;
    struct list loaded_pages;
    uint16_t i;

    if (directory == NULL ||
        image == NULL ||
        entry == NULL)
        return 0;

    if (!elf_validate(image, size))
        return 0;

    header = (const struct elf32_header *)image;

    list_init(&loaded_pages);

    for (i = 0; i < header->e_phnum; i++)
    {
        const struct elf32_program_header *ph;
        uintptr_t segment_start;
        uintptr_t segment_end;
        uintptr_t page;
        uintptr_t segment_offset;
        uint32_t flags;

        ph = (const struct elf32_program_header *)
        ((const uint8_t *)image +
        header->e_phoff +
        ((uint32_t)i * header->e_phentsize));

        if (ph->p_type != PT_LOAD)
            continue;

        /*
         * The ELF file cannot contain more bytes than
         * the segment will occupy in memory.
         */
        if (ph->p_memsz < ph->p_filesz)
            goto fail;

        /*
         * Make sure the file portion of the segment is
         * actually inside the ELF image.
         */
        if (ph->p_offset > size ||
            ph->p_filesz > size - ph->p_offset)
            goto fail;

        /*
         * Detect virtual-address overflow.
         */
        if (ph->p_vaddr + ph->p_memsz < ph->p_vaddr)
            goto fail;

        /*
         * Calculate the page-aligned virtual range.
         */
        segment_start =
        ph->p_vaddr & ~(PAGE_SIZE - 1);

        segment_end =
        (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1)
        & ~(PAGE_SIZE - 1);

        /*
         * ELF flags:
         *
         * PF_W -> writable
         * Otherwise the segment is read-only.
         *
         * Every user segment is present and user-accessible.
         */
        flags = PAGE_PRESENT | PAGE_USER;

        if (ph->p_flags & PF_W)
            flags |= PAGE_WRITABLE;

        /*
         * Offset of the segment inside its first page.
         */
        segment_offset =
        ph->p_vaddr - segment_start;

        /*
         * Allocate and map every page belonging to
         * this PT_LOAD segment.
         */
        for (page = segment_start;
             page < segment_end;
             page += PAGE_SIZE)
             {
                 struct elf_loaded_page *loaded;
                 void *phys;
                 uintptr_t translated;
                 uint32_t page_offset;
                 uint32_t copy_offset;
                 uint32_t copy_size;

                 /*
                  * Allocate a physical page.
                  */
                 phys = pmm_alloc_page();

                 if (phys == NULL)
                 {
                     printf("ELF: physical page allocation failed\n");
                     goto fail;
                 }

                 /*
                  * Zero the entire physical page.
                  *
                  * This also handles the BSS portion of a segment
                  * where p_memsz > p_filesz.
                  */
                 memset(
                     (void *)PHYS_TO_VIRT((uintptr_t)phys),
                        0,
                        PAGE_SIZE
                 );

                 /*
                  * Map the physical page into the process
                  * address space.
                  */
                 paging_map(
                     directory,
                     page,
                     (uintptr_t)phys,
                     flags
                 );

                 /*
                  * Verify that the mapping was actually installed.
                  */
                 translated =
                 paging_translate(
                     directory,
                     page
                 );

                 if (translated != (uintptr_t)phys)
                 {
                     printf(
                         "ELF: failed to map virtual page %x\n",
                         (uint32_t)page
                     );

                     paging_unmap(
                         directory,
                         page
                     );

                     pmm_free_page(phys);

                     goto fail;
                 }

                 loaded = kmalloc(sizeof(*loaded));

                 if (loaded == NULL)
                 {
                     paging_unmap(
                         directory,
                         page
                     );

                     pmm_free_page(phys);

                     goto fail;
                 }

                 loaded->virt = page;

                 list_node_init(&loaded->node);

                 list_push_back(
                     &loaded_pages,
                     &loaded->node
                 );

                 /*
                  * Determine where the ELF file data starts
                  * within this page.
                  *
                  * The first page may start in the middle because
                  * p_vaddr does not necessarily have to be page-aligned.
                  */
                 page_offset = 0;

                 if (page == segment_start)
                     page_offset = (uint32_t)segment_offset;

                 /*
                  * Offset into the segment's file data.
                  */
                 copy_offset =
                 (uint32_t)(page - segment_start) +
                 page_offset;

                 /*
                  * Calculate how many bytes from the ELF file
                  * belong in this page.
                  */
                 if (copy_offset >= ph->p_filesz)
                 {
                     copy_size = 0;
                 }
                 else
                 {
                     copy_size =
                     ph->p_filesz - copy_offset;

                     if (copy_size >
                         PAGE_SIZE - page_offset)
                     {
                         copy_size =
                         PAGE_SIZE - page_offset;
                     }
                 }

                 /*
                  * Copy the file-backed portion into the
                  * physical page through the kernel's
                  * higher-half physical mapping.
                  */
                 if (copy_size != 0)
                 {
                     memcpy(
                         (uint8_t *)
                         PHYS_TO_VIRT((uintptr_t)phys)
                         + page_offset,

                         (const uint8_t *)image
                         + ph->p_offset
                         + copy_offset,

                         copy_size
                     );
                 }
             }
    }

    /*
     * The ELF entry point is the virtual address at which
     * the new user process must begin execution.
     */
    *entry = header->e_entry;

    printf(
        "ELF loaded: entry=%x\n",
        (uint32_t)*entry
    );

    return 1;

    fail:
    while (!list_empty(&loaded_pages))
    {
        struct list_node *node;
        struct elf_loaded_page *loaded;
        uintptr_t phys;

        node = list_back(&loaded_pages);

        loaded = container_of(
            node,
            struct elf_loaded_page,
            node
        );

        phys = paging_translate(
            directory,
            loaded->virt
        );

        list_remove(node);

        paging_unmap(
            directory,
            loaded->virt
        );

        if (phys != 0)
            pmm_free_page((void *)phys);

        kfree(loaded);
    }

    return 0;

}

// dumpers
void elf_dump_load_segments(const void *image)
{
    const struct elf32_header *header;
    const struct elf32_program_header *ph;
    uint16_t i;

    header = (const struct elf32_header *)image;

    for (i = 0; i < header->e_phnum; i++)
    {
        ph = (const struct elf32_program_header *)
        ((const uint8_t *)image +
        header->e_phoff +
        ((uint32_t)i * header->e_phentsize));

        if (ph->p_type != PT_LOAD)
            continue;

        printf("PT_LOAD:\n");
        printf("  offset = %x\n", ph->p_offset);
        printf("  vaddr  = %x\n", ph->p_vaddr);
        printf("  filesz = %x\n", ph->p_filesz);
        printf("  memsz  = %x\n", ph->p_memsz);
        printf("  flags  = %x\n", ph->p_flags);
        printf("  align  = %x\n", ph->p_align);
    }
}
