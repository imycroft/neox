#ifndef MEMORY_LAYOUT_H
#define MEMORY_LAYOUT_H

#include "types.h"
/*
 * ============================================================================
 * Neox Virtual Memory Layout
 * ============================================================================
 *
 * 32-bit x86, 4 GiB virtual address space.
 *
 * The virtual address space is divided into:
 *
 *   0x00000000 - 0xBFFFFFFF : User virtual address space
 *   0xC0000000 - 0xFFBFFFFF : Kernel virtual address space
 *   0xFFC00000 - 0xFFFFFFFF : Recursive page-directory mapping
 *
 * User space is process-specific.
 *
 * Kernel space is shared by all processes.
 *
 * The final 4 MiB are permanently reserved for recursive paging
 * using PDE 1023.
 *
 * This file is the architectural source of truth for the virtual
 * address-space layout.
 *
 * Other memory-management code should use these constants instead of
 * introducing ad-hoc virtual-address ranges.
 *
 * ============================================================================
 */


/*
 * ============================================================================
 * Entire 32-bit virtual address space
 * ============================================================================
 *
 * 32-bit x86 provides:
 *
 *     2^32 = 4 GiB
 *
 * virtual addresses.
 */

#define VIRTUAL_ADDRESS_SPACE_START   0x00000000u
#define VIRTUAL_ADDRESS_SPACE_END     0xFFFFFFFFu
#define VIRTUAL_ADDRESS_SPACE_SIZE    0x100000000ULL


/*
 * ============================================================================
 * User virtual address space
 * ============================================================================
 *
 * Each process owns its own mappings in this region.
 *
 *     0x00000000 - 0xBFFFFFFF
 *
 * Size:
 *
 *     0xC0000000 = 3 GiB
 *
 * PDEs:
 *
 *     PDE 0 - 767
 *
 * Each PDE covers 4 MiB.
 *
 * The first page is deliberately kept unmapped as a NULL-pointer guard.
 */

#define USER_VIRT_START              0x00000000u
#define USER_VIRT_END                0xBFFFFFFFu
#define USER_VIRT_SIZE               0xC0000000u

#define USER_VIRT_NULL_GUARD_END     0x00000FFFu


/*
 * ============================================================================
 * Kernel virtual address space
 * ============================================================================
 *
 * The kernel occupies the upper portion of the address space:
 *
 *     0xC0000000 - 0xFFBFFFFF
 *
 * Size:
 *
 *     0x3FC00000 = 1020 MiB
 *
 * PDEs:
 *
 *     PDE 768 - 1022
 *
 * These mappings are shared by all processes.
 *
 * PDE 1023 is NOT part of the kernel allocation space because it is
 * permanently reserved for recursive paging.
 */

#define KERNEL_VIRT_BASE             0xC0000000u
#define KERNEL_VIRT_END              0xFFBFFFFFu
#define KERNEL_VIRT_SIZE             0x3FC00000u

#define BOOTSTRAP_MAP_SIZE           0x01000000u

#define KERNEL_BOOTSTRAP_END         \
(KERNEL_VIRT_BASE + BOOTSTRAP_MAP_SIZE)

/*
 * ============================================================================
 * Recursive page-table mapping
 * ============================================================================
 *
 * PDE 1023 maps the page directory onto itself.
 *
 * This consumes the final 4 MiB of the virtual address space:
 *
 *     0xFFC00000 - 0xFFFFFFFF
 *
 * Size:
 *
 *     4 MiB
 *
 * PDE 1023:
 *
 *     1023 * 4 MiB = 0xFFC00000
 *
 * This allows the paging subsystem to access page directories and page
 * tables through virtual addresses without requiring their physical
 * addresses to be directly accessible.
 */

#define RECURSIVE_PDE_INDEX          1023u

#define RECURSIVE_VIRT_BASE          0xFFC00000u
#define RECURSIVE_VIRT_END           0xFFFFFFFFu
#define RECURSIVE_VIRT_SIZE          0x00400000u


/*
 * ============================================================================
 * Kernel image
 * ============================================================================
 *
 * The kernel is physically loaded at KERNEL_LOAD_ADDRESS but linked to
 * execute in the higher-half kernel address space.
 *
 * The linker determines the actual end of the kernel image.
 *
 * These constants therefore describe the physical load address and the
 * virtual link address of the beginning of the kernel image.
 */

#define KERNEL_LOAD_ADDRESS          0x00100000u
#define KERNEL_LINK_ADDRESS          0xC0100000u

#define KERNEL_IMAGE_BASE            KERNEL_LINK_ADDRESS


/*
 * ============================================================================
 * Kernel physical/virtual offset
 * ============================================================================
 *
 * The kernel image is linked KERNEL_PHYS_OFFSET bytes above its physical
 * load address.
 *
 * For the current layout:
 *
 *     0xC0100000 - 0x00100000 = 0xC0000000
 *
 * This offset is valid for the kernel's higher-half direct mapping.
 *
 * It must NOT be assumed to apply to arbitrary kernel heap, MMIO, or
 * dynamically mapped virtual addresses.
 */

#define KERNEL_PHYS_OFFSET \
(KERNEL_LINK_ADDRESS - KERNEL_LOAD_ADDRESS)


/*
 * Convert between physical and virtual addresses within the kernel's
 * higher-half direct mapping.
 *
 * These macros are only valid for addresses belonging to that mapping.
 */

#define PHYS_TO_VIRT(addr) \
((uintptr_t)(addr) + KERNEL_PHYS_OFFSET)

#define VIRT_TO_PHYS(addr) \
((uintptr_t)(addr) - KERNEL_PHYS_OFFSET)


/*
 * ============================================================================
 * Kernel heap
 * ============================================================================
 *
 * Reserved virtual-address region for the kernel heap.
 *
 *     0xC0800000 - 0xCFFFFFFF
 *
 * Size:
 *
 *     0x0F800000 = 248 MiB
 *
 * The range is inclusive, therefore:
 *
 *     END - START + 1 = SIZE
 *
 * This is a virtual-address reservation. Physical pages backing the heap
 * are managed separately by the physical memory manager.
 */

#define KERNEL_HEAP_START            KERNEL_BOOTSTRAP_END
#define KERNEL_HEAP_END        0xCFFFFFFFu
#define KERNEL_HEAP_SIZE       \
(KERNEL_HEAP_END - KERNEL_HEAP_START + 1u)

/*
 * ============================================================================
 * Kernel stacks
 * ============================================================================
 *
 * Reserved virtual-address region for kernel stacks.
 *
 *     0xD0000000 - 0xDFFFFFFF
 *
 * Size:
 *
 *     0x10000000 = 256 MiB
 *
 * Kernel stacks should be allocated from this region.
 *
 * Guard pages should remain unmapped between stack allocations or at
 * appropriate stack boundaries so that stack overflow can trigger a
 * page fault during development.
 */

#define KERNEL_STACKS_START          0xD0000000u
#define KERNEL_STACKS_END            0xDFFFFFFFu
#define KERNEL_STACKS_SIZE           0x10000000u


/*
 * ============================================================================
 * Remaining kernel virtual-address region
 * ============================================================================
 *
 * This region is available for kernel mappings that are not part of the
 * fixed kernel image, heap, or kernel-stack regions.
 *
 * Possible future uses include:
 *
 *   - MMIO mappings
 *   - dynamically mapped physical memory
 *   - page-table management structures
 *   - other kernel virtual mappings
 *
 *     0xE0000000 - 0xFFBFFFFF
 *
 * The exact sub-layout of this region has not yet been finalized.
 *
 * Do not assume that the entire region is currently free. Future memory
 * subsystems must explicitly reserve portions as their architecture
 * is defined.
 */

#define KERNEL_REMAINING_START       0xE0000000u
#define KERNEL_REMAINING_END         0xFFBFFFFFu
#define KERNEL_REMAINING_SIZE        0x1FC00000u


/*
 * ============================================================================
 * Page-directory constants
 * ============================================================================
 *
 * A 32-bit x86 page directory contains 1024 PDEs.
 *
 * Each PDE covers 4 MiB of virtual address space when using 4 KiB pages.
 */

#define PAGE_DIRECTORY_ENTRIES       1024u
#define PAGE_TABLE_ENTRIES           1024u

#define PAGE_DIRECTORY_COVERAGE      0x00400000u


/*
 * Number and range of PDEs belonging to user and kernel space.
 */

#define USER_PDE_COUNT               768u

#define KERNEL_PDE_START             768u
#define KERNEL_PDE_END               1022u

/*
 * PDE 1023 is permanently reserved for recursive paging.
 */

#define RECURSIVE_PDE_COUNT          1u


/*
 * ============================================================================
 * Kernel stack virtual-address region
 * ============================================================================
 */


#define KERNEL_STACK_SIZE    0x4000u   /* 16 KiB per kernel stack */

/*
 * PDE containing the kernel stack virtual-address region.
 *
 * KERNEL_STACKS_START = 0xD0000000
 * Therefore this is PDE 832.
 */
#define KERNEL_STACK_PDE (KERNEL_STACKS_START >> 22)

#define KERNEL_STACK_COUNT \
((KERNEL_STACKS_END - KERNEL_STACKS_START + 1u) / \
KERNEL_STACK_SIZE)

#define KERNEL_STACK_BITMAP_BYTES \
((KERNEL_STACK_COUNT + 7u) / 8u)

/*
 * ============================================================================
 * Compile-time layout validation
 * ============================================================================
 *
 * These assertions turn the architectural relationships above into compiler-
 * enforced invariants.
 *
 * uint64_t-style arithmetic is used for the +1 checks so that:
 *
 *     0xFFFFFFFF + 1
 *
 * does not wrap around a 32-bit unsigned integer.
 *
 * Neox is using C23, so _Static_assert is available.
 */


/*
 * User space must end immediately before kernel space.
 */

_Static_assert(
    (uint64_t)USER_VIRT_END + 1ULL == KERNEL_VIRT_BASE,
               "user/kernel virtual address boundary is invalid"
);


/*
 * Kernel space must end immediately before recursive paging.
 */

_Static_assert(
    (uint64_t)KERNEL_VIRT_END + 1ULL == RECURSIVE_VIRT_BASE,
               "kernel/recursive virtual address boundary is invalid"
);


/*
 * Recursive PDE 1023 must begin at the corresponding 4 MiB boundary.
 */

_Static_assert(
    RECURSIVE_PDE_INDEX * PAGE_DIRECTORY_COVERAGE ==
    RECURSIVE_VIRT_BASE,
    "recursive PDE does not match recursive virtual address"
);


/*
 * The recursive region must occupy exactly one PDE.
 */

_Static_assert(
    RECURSIVE_VIRT_SIZE == PAGE_DIRECTORY_COVERAGE,
    "recursive virtual region must be exactly one PDE"
);


/*
 * User space must contain exactly 768 PDEs.
 */

_Static_assert(
    USER_PDE_COUNT * PAGE_DIRECTORY_COVERAGE == USER_VIRT_SIZE,
    "user virtual space does not match user PDE count"
);


/*
 * The kernel PDE range must end immediately before PDE 1023.
 */

_Static_assert(
    KERNEL_PDE_END + 1 == RECURSIVE_PDE_INDEX,
    "kernel PDE range overlaps recursive PDE"
);


/*
 * Kernel heap size must match its inclusive address range.
 */

_Static_assert(
    (uint64_t)KERNEL_HEAP_END -
    (uint64_t)KERNEL_HEAP_START + 1ULL ==
    KERNEL_HEAP_SIZE,
    "kernel heap size does not match its address range"
);


/*
 * Kernel stack size must match its inclusive address range.
 */

_Static_assert(
    (uint64_t)KERNEL_STACKS_END -
    (uint64_t)KERNEL_STACKS_START + 1ULL ==
    KERNEL_STACKS_SIZE,
    "kernel stack size does not match its address range"
);


/*
 * Remaining kernel region must match its inclusive address range.
 */

_Static_assert(
    (uint64_t)KERNEL_REMAINING_END -
    (uint64_t)KERNEL_REMAINING_START + 1ULL ==
    KERNEL_REMAINING_SIZE,
    "remaining kernel region size does not match its address range"
);


/*
 * The kernel physical/virtual offset must be consistent with the selected
 * load and link addresses.
 */

_Static_assert(
    KERNEL_LINK_ADDRESS - KERNEL_LOAD_ADDRESS == KERNEL_PHYS_OFFSET,
    "kernel physical/virtual offset is inconsistent"
);

/* Kernel image base must be the start of kernel virtual space */
_Static_assert(
    KERNEL_IMAGE_BASE == KERNEL_VIRT_BASE + 0x00100000u,
    "kernel image does not start at expected offset"
);

/* Heap must follow the kernel image region */
_Static_assert(
    KERNEL_HEAP_START > KERNEL_IMAGE_BASE,
    "kernel heap overlaps kernel image region"
);

/* Stacks must immediately follow heap */
_Static_assert(
    KERNEL_STACKS_START == KERNEL_HEAP_END + 1,
    "gap or overlap between kernel heap and stacks"
);

/* Remaining must immediately follow stacks */
_Static_assert(
    KERNEL_REMAINING_START == KERNEL_STACKS_END + 1,
    "gap or overlap between kernel stacks and remaining region"
);

/* Remaining must end immediately before recursive region */
_Static_assert(
    (uint64_t)KERNEL_REMAINING_END + 1ULL == RECURSIVE_VIRT_BASE,
               "gap or overlap between remaining region and recursive paging"
);

/*
 * Every PDE must be accounted for by exactly one region.
 */

_Static_assert(
    KERNEL_PDE_END + RECURSIVE_PDE_COUNT + 1 == PAGE_DIRECTORY_ENTRIES,
    "PDE ranges do not account for the full page directory"
);

_Static_assert(
    USER_PDE_COUNT + (KERNEL_PDE_END - KERNEL_PDE_START + 1) + RECURSIVE_PDE_COUNT
    == PAGE_DIRECTORY_ENTRIES,
    "PDE region counts do not sum to full page directory"
);

_Static_assert(
    KERNEL_HEAP_START >= KERNEL_BOOTSTRAP_END,
    "kernel heap overlaps bootstrap mapping"
);

_Static_assert(
    KERNEL_HEAP_END < KERNEL_STACKS_START,
    "kernel heap overlaps kernel stack region"
);

#endif /* MEMORY_LAYOUT_H */

