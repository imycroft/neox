/*
 * ============================================================================
 * NeOx kernel linker script
 * ============================================================================
 *
 * Physical load address:   0x00100000
 * Kernel virtual address:  0xC0100000
 * Higher-half offset:      0xC0000000
 *
 * ELF segment layout (explicit PHDRS)
 * ------------------------------------
 *
 *   seg_mb   PT_LOAD  VMA=0x00100000 LMA=0x00100000  .multiboot
 *   seg_kern PT_LOAD  VMA=0xC0100000 LMA=0x00101000  .text .rodata .data .bss
 *
 * All higher-half sections are assigned to seg_kern via the :seg_kern
 * notation.  This prevents the linker from splitting them into multiple
 * PT_LOAD segments, which is what causes GRUB's overlap error.
 *
 * BSS is NOBITS so the linker sets MemSiz > FileSiz on seg_kern.
 * GRUB zero-fills the difference; no file bytes are emitted for BSS.
 *
 * ============================================================================
 */

ENTRY(start)

KERNEL_PHYS_BASE  = 0x00100000;
KERNEL_VIRT_BASE  = 0xC0000000;
KERNEL_VIRT_START = 0xC0101000;


PHDRS
{
    seg_mb   PT_LOAD FLAGS(4);        /* R   — multiboot header           */
    seg_kern PT_LOAD FLAGS(7);        /* RWX — kernel text + data + bss   */
    seg_stack PT_GNU_STACK FLAGS(6);  /* RW  — marks stack non-executable */
}


SECTIONS
{
    /*
     * ------------------------------------------------------------------------
     * Multiboot header
     * ------------------------------------------------------------------------
     */

    . = KERNEL_PHYS_BASE;

    .multiboot ALIGN(8) : AT(KERNEL_PHYS_BASE)
    {
        KEEP(*(.multiboot))
    } :seg_mb


    /*
     * ------------------------------------------------------------------------
     * Kernel text
     * ------------------------------------------------------------------------
     *
     * LMA = KERNEL_PHYS_BASE + 0x1000 places .text on the page immediately
     * after the multiboot header, with no physical overlap.
     */

    . = KERNEL_VIRT_START;

    .text ALIGN(0x1000) : AT(KERNEL_PHYS_BASE + 0x1000)
    {
        kernel_virtual_start = .;

        *(.text.start)
        *(.text)
        *(.text.*)

        kernel_text_end = .;
    } :seg_kern

    /*
     * ------------------------------------------------------------------------
     * Read-only data
     * ------------------------------------------------------------------------
     */

    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)

        kernel_rodata_end = .;
    } :seg_kern


    /*
     * ------------------------------------------------------------------------
     * Initialized data
     * ------------------------------------------------------------------------
     */

    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)

        kernel_data_end = .;
    } :seg_kern


    /*
     * ------------------------------------------------------------------------
     * BSS
     * ------------------------------------------------------------------------
     *
     * NOBITS — no file bytes.  Assigned to seg_kern so the linker extends
     * that segment's MemSiz rather than creating a new PT_LOAD.
     */

    .bss ALIGN(0x1000) :
    {
        kernel_bss_start = .;

        *(COMMON)
        *(.bss)
        *(.bss.*)

        . = ALIGN(0x1000);
        kernel_bss_end = .;
    } :seg_kern


    /*
     * =========================================================================
     * Kernel boundary symbols
     * =========================================================================
     */

    /* Virtual addresses*/
    kernel_virtual_start = ADDR(.text);
    kernel_virtual_end   = kernel_bss_end;

    /* Physical addresses (LMA) */
    kernel_physical_start = LOADADDR(.text);
    kernel_physical_end   = LOADADDR(.text)
                            + (kernel_virtual_end - kernel_virtual_start);

    kernel_start = kernel_physical_start;
    kernel_end   = kernel_physical_end;

    /*
     * =========================================================================
     * Discarded sections
     * =========================================================================
     */

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
