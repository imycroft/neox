ENTRY(start)

SECTIONS {
    . = 0x00100000;
    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }
    .text ALIGN(0X1000) :
    {
        *(.text)
    }

    .rodata ALIGN(0X1000) :
    {
        *(.rodata)
    }

    .data ALIGN(0X1000) :
    {
        *(.data)
    }

    .bss ALIGN(0X1000) :
    {
        *(COMMON)
        *(.bss)
    }
}
