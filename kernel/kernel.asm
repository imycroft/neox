BITS 32

global start
global kernel_end

extern kernel_main

KERNEL_STACK_SIZE equ 4096

MB2_MAGIC        equ 0xE85250D6
MB2_ARCHITECTURE equ 0

MB2_HEADER_LENGTH equ header_end - header_start
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCHITECTURE + MB2_HEADER_LENGTH)
MB2_BOOTLOADER_MAGIC equ 0x36D76289

section .multiboot
align 8

header_start:

    dd MB2_MAGIC
    dd MB2_ARCHITECTURE
    dd MB2_HEADER_LENGTH
    dd MB2_CHECKSUM

    ; End tag

    dw 0
    dw 0
    dd 8

header_end:

section .text

start:

    mov esp, kernel_stack + KERNEL_STACK_SIZE

    cmp eax, MB2_BOOTLOADER_MAGIC
    jne hang

    push ebx        ; Multiboot2 information pointer
    push eax        ; Multiboot2 magic

    call kernel_main

    add esp, 8

hang:
    hlt
    jmp hang

section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

kernel_end:
section .note.GNU-stack noalloc
