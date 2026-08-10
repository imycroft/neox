bits 32

section .text

extern syscall_handler

global syscall_stub

;-------------------------------------------------------
; INT 0x80 entry point — called from Ring 3.
;
; The CPU has already pushed (from the Ring-3 stack, in order):
;   SS, ESP (user), EFLAGS, CS (user), EIP (user)
; and switched to the kernel stack (ss0:esp0 from the TSS).
;
; We push a dummy error code and int_no=0x80, then fall into
; the common ISR path to build a full `struct registers`.
;-------------------------------------------------------

syscall_stub:
    cli
    push dword 0        ; dummy error code
    push dword 0x80     ; int_no

    pusha

    push ds
    push es
    push fs
    push gs

    ; Switch data segments to kernel selectors.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pointer to struct registers
    call syscall_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa

    add esp, 8          ; pop int_no + err_code

    iret
