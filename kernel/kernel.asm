BITS 32

global start

extern kernel_main
extern kernel_physical_end



; ============================================================================
; Kernel address layout
; ============================================================================

KERNEL_LOAD_ADDRESS equ 0x00100000
KERNEL_LINK_ADDRESS equ 0xC0100000

; Difference between the kernel's virtual and physical addresses.
;
; Physical:
;     0x00100000
;
; Virtual:
;     0xC0100000
;
; Therefore:
;     virtual = physical + 0xC0000000
;
KERNEL_PHYS_OFFSET equ KERNEL_LINK_ADDRESS - KERNEL_LOAD_ADDRESS

; Convert a higher-half linked symbol into its physical address.
;
; This is used ONLY before paging is enabled.
%define PHYS_ADDR(symbol) (symbol - KERNEL_PHYS_OFFSET)


; ============================================================================
; Paging constants
; ============================================================================

PAGE_SIZE        equ 4096
PAGE_ENTRIES     equ 1024
PAGE_TABLE_SIZE  equ PAGE_SIZE * PAGE_ENTRIES

; PDE 768 corresponds to:
;
;     768 * 4 MiB = 0xC0000000
;
; This is the beginning of the higher-half kernel address space.
KERNEL_PDE_INDEX equ 768


; ============================================================================
; Bootstrap stack
; ============================================================================

KERNEL_STACK_SIZE equ 4096 * 1024


; ============================================================================
; Multiboot 2 constants
; ============================================================================

MB2_MAGIC        equ 0xE85250D6
MB2_ARCHITECTURE equ 0

MB2_HEADER_LENGTH equ header_end - header_start

MB2_CHECKSUM \
    equ -(MB2_MAGIC + MB2_ARCHITECTURE + MB2_HEADER_LENGTH)

MB2_BOOTLOADER_MAGIC equ 0x36D76289


; ============================================================================
; Multiboot 2 header
; ============================================================================

section .multiboot
align 8

header_start:

    dd MB2_MAGIC
    dd MB2_ARCHITECTURE
    dd MB2_HEADER_LENGTH
    dd MB2_CHECKSUM

    ; ------------------------------------------------------------------------
    ; End tag
    ; ------------------------------------------------------------------------

    dw 0
    dw 0
    dd 8

header_end:


; ============================================================================
; Bootstrap code
; ============================================================================
;
; This code executes while paging is disabled.
;
; Therefore:
;
;     - addresses must refer to physical/identity-mapped memory
;     - CR3 must receive a physical address
;     - page-directory/table entries must contain physical addresses
;
; The bootstrap creates temporary mappings:
;
;     Physical 0x00000000 -> Virtual 0x00000000
;
; and:
;
;     Physical 0x00000000 -> Virtual 0xC0000000
;
; for the first 16 MiB.
;
; This allows execution to transition from the low physical address
; where GRUB loaded the kernel to the higher-half virtual address where
; the kernel is linked.
;
; ============================================================================

section .text.start

start:

    ; ------------------------------------------------------------------------
    ; Establish the temporary bootstrap stack.
    ;
    ; Paging is disabled here, so bootstrap_stack is converted to its
    ; physical address.
    ; ------------------------------------------------------------------------

    mov esp, PHYS_ADDR(bootstrap_stack) + KERNEL_STACK_SIZE


    ; ------------------------------------------------------------------------
    ; Verify that we were entered by a Multiboot 2 bootloader.
    ;
    ; EAX contains the Multiboot magic.
    ; EBX contains the physical address of the Multiboot information.
    ; ------------------------------------------------------------------------

    cmp eax, MB2_BOOTLOADER_MAGIC
    jne hang


    ; ------------------------------------------------------------------------
    ; Preserve the Multiboot arguments.
    ;
    ; We cannot rely on general-purpose registers surviving the paging
    ; setup because they are heavily used while constructing the tables.
    ;
    ; These variables are in .bss and are part of the bootstrap mapping.
    ; ------------------------------------------------------------------------

    mov [PHYS_ADDR(bootstrap_mb_magic)], eax
    mov [PHYS_ADDR(bootstrap_mb_info)], ebx



    ; =========================================================================
    ; Clear bootstrap page directory
    ; =========================================================================

    mov edi, PHYS_ADDR(bootstrap_page_directory)

    xor eax, eax
    mov ecx, PAGE_ENTRIES

    rep stosd


    ; =========================================================================
    ; Install identity-mapped page tables
    ; =========================================================================
    ;
    ; PDE 0-3 cover:
    ;
    ;     0x00000000 - 0x00FFFFFF
    ;
    ; Each PDE covers 4 MiB.
    ;
    ; These mappings allow the CPU to continue executing the bootstrap
    ; immediately after CR0.PG is enabled.
    ;
    ; ========================================================================

    mov eax, PHYS_ADDR(bootstrap_page_table0)
    or eax, 0x003                       ; PRESENT | WRITABLE
    mov [PHYS_ADDR(bootstrap_page_directory) + 0 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table1)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 1 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table2)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 2 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table3)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 3 * 4], eax


    ; =========================================================================
    ; Install higher-half mappings
    ; =========================================================================
    ;
    ; PDE 768 corresponds to virtual address 0xC0000000.
    ;
    ; We point PDE 768-771 at the SAME page tables used by PDE 0-3.
    ;
    ; Therefore:
    ;
    ;     0x00000000 -> physical 0x00000000
    ;
    ; and:
    ;
    ;     0xC0000000 -> physical 0x00000000
    ;
    ; refer to the same physical memory.
    ;
    ; ========================================================================

    mov eax, PHYS_ADDR(bootstrap_page_table0)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 768 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table1)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 769 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table2)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 770 * 4], eax

    mov eax, PHYS_ADDR(bootstrap_page_table3)
    or eax, 0x003
    mov [PHYS_ADDR(bootstrap_page_directory) + 771 * 4], eax


    ; =========================================================================
    ; Fill the four bootstrap page tables
    ; =========================================================================
    ;
    ; Each page table contains 1024 entries.
    ;
    ; Each entry maps one 4 KiB page.
    ;
    ; Therefore each table maps 4 MiB.
    ;
    ; Table 0: 0x00000000 - 0x003FFFFF
    ; Table 1: 0x00400000 - 0x007FFFFF
    ; Table 2: 0x00800000 - 0x00BFFFFF
    ; Table 3: 0x00C00000 - 0x00FFFFFF
    ;
    ; ========================================================================

    xor esi, esi

    mov edi, PHYS_ADDR(bootstrap_page_table0)
    call fill_page_table

    mov edi, PHYS_ADDR(bootstrap_page_table1)
    mov esi, 0x00400000
    call fill_page_table

    mov edi, PHYS_ADDR(bootstrap_page_table2)
    mov esi, 0x00800000
    call fill_page_table

    mov edi, PHYS_ADDR(bootstrap_page_table3)
    mov esi, 0x00C00000
    call fill_page_table


    ; =========================================================================
    ; Load the bootstrap page directory
    ; =========================================================================
    ;
    ; CR3 always receives the PHYSICAL address of the page directory.
    ;
    ; ========================================================================

    mov eax, PHYS_ADDR(bootstrap_page_directory)
    mov cr3, eax


    ; =========================================================================
    ; Enable paging
    ; =========================================================================
    ;
    ; CR0.PG = bit 31.
    ;
    ; After this instruction, both of the following addresses are valid:
    ;
    ;     0x00000000 -> physical 0x00000000
    ;     0xC0000000 -> physical 0x00000000
    ;
    ; The current instruction continues executing through the identity
    ; mapping until we explicitly switch to the higher-half stack and call
    ; kernel_main().
    ;
    ; ========================================================================

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax


    ; =========================================================================
    ; Switch to the higher-half kernel stack
    ; =========================================================================
    ;
    ; Paging is now enabled, so kernel_stack's HIGH virtual address is valid.
    ;
    ; The linker places kernel_stack in the higher-half kernel address space.
    ;
    ; ========================================================================

    mov esp, kernel_stack + KERNEL_STACK_SIZE


    ; =========================================================================
    ; Enter the higher-half kernel
    ; =========================================================================
    ;
    ; kernel_main() is linked at a higher-half virtual address.
    ;
    ; The Multiboot values were saved before we destroyed the contents of
    ; the general-purpose registers during page-table construction.
    ;
    ; cdecl arguments are pushed right-to-left:
    ;
    ;     kernel_main(magic, mb_info)
    ;
    ; therefore:
    ;
    ;     push mb_info
    ;     push magic
    ;
    ; ========================================================================

    push dword [bootstrap_mb_info]
    push dword [bootstrap_mb_magic]

    ;mov eax, kernel_main ; temp

    call kernel_main

    add esp, 8


; ============================================================================
; Bootstrap page-table helper
; ============================================================================
;
; Input:
;
;     EDI = physical address of page table
;     ESI = physical address mapped by the first entry
;
; The function creates 1024 4-KiB mappings.
;
; Example:
;
;     ESI = 0x00400000
;
; produces:
;
;     entry 0    -> 0x00400000
;     entry 1    -> 0x00401000
;     ...
;     entry 1023 -> 0x007FF000
;
; ============================================================================

fill_page_table:

    push eax
    push ecx
    push edi

    mov eax, esi
    or eax, 0x003                       ; PRESENT | WRITABLE

    mov ecx, PAGE_ENTRIES

.fill:

    mov [edi], eax

    add eax, PAGE_SIZE
    add edi, 4

    loop .fill

    pop edi
    pop ecx
    pop eax

    ret


; ============================================================================
; Hang
; ============================================================================

hang:

    cli

.hang:
    hlt
    jmp .hang


; ============================================================================
; Bootstrap data
; ============================================================================
;
; These objects are placed in .bss.
;
; The linker gives the kernel's normal sections higher-half virtual
; addresses. PHYS_ADDR() is therefore required whenever these objects
; are accessed BEFORE paging is enabled.
;
; ============================================================================

section .bss

align 4

; ---------------------------------------------------------------------------
; Saved Multiboot arguments
; ---------------------------------------------------------------------------

bootstrap_mb_magic:
    resd 1

bootstrap_mb_info:
    resd 1


; ---------------------------------------------------------------------------
; Temporary bootstrap stack
; ---------------------------------------------------------------------------

alignb PAGE_SIZE

bootstrap_stack:
    resb KERNEL_STACK_SIZE


; ---------------------------------------------------------------------------
; Temporary bootstrap page directory
; ---------------------------------------------------------------------------

alignb PAGE_SIZE

bootstrap_page_directory:
    resb PAGE_SIZE


; ---------------------------------------------------------------------------
; Temporary bootstrap page tables
; ---------------------------------------------------------------------------

bootstrap_page_table0:
    resb PAGE_SIZE

bootstrap_page_table1:
    resb PAGE_SIZE

bootstrap_page_table2:
    resb PAGE_SIZE

bootstrap_page_table3:
    resb PAGE_SIZE


; ---------------------------------------------------------------------------
; Normal higher-half kernel stack
; ---------------------------------------------------------------------------

kernel_stack:
    resb KERNEL_STACK_SIZE


section .note.GNU-stack noalloc
