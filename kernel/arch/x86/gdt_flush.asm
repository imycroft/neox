bits 32

global gdt_load

CODE_SELECTOR equ 0x08
DATA_SELECTOR equ 0x10

section .text

gdt_load:
    ; Argument:
    ;   [esp + 4] = struct gdt_ptr *

    mov eax, [esp + 4]

    ; Load the GDTR
    lgdt [eax]

    ; Reload data segment registers
    mov ax, DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS
    jmp CODE_SELECTOR:.flush

.flush:
    ret
