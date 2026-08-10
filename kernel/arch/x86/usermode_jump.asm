bits 32

global jump_usermode

;-------------------------------------------------------
; void jump_usermode(uintptr_t user_esp,
;                    uintptr_t user_entry)
;
; Switches the CPU from Ring 0 to Ring 3 via IRET.
;
; IRET frame (pushed in reverse order — high addr first):
;   [esp+0]  EIP    (user_entry)
;   [esp-4]  CS     (GDT_USER_CODE_SELECTOR = 0x1B)
;   [esp-8]  EFLAGS (IF set so interrupts work in user mode)
;   [esp-12] ESP    (user_esp)
;   [esp-16] SS     (GDT_USER_DATA_SELECTOR = 0x23)
;-------------------------------------------------------

USER_CODE_SEL equ 0x1B   ; GDT_USER_CODE_SELECTOR (0x18 | 3)
USER_DATA_SEL equ 0x23   ; GDT_USER_DATA_SELECTOR (0x20 | 3)
EFLAGS_IF     equ 0x200  ; Interrupt Enable flag

section .text

jump_usermode:
    ; Arguments on stack (cdecl, 32-bit):
    ;   [esp + 4]  = user_esp
    ;   [esp + 8]  = user_entry
    mov eax, [esp + 4]   ; user_esp
    mov ecx, [esp + 8]   ; user_entry

    ; Load user-mode data segments into DS/ES/FS/GS so that
    ; user-mode code starts with correct segment registers.
    mov dx, USER_DATA_SEL
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    ; Build the IRET frame on the current (kernel) stack.
    push USER_DATA_SEL   ; SS  (user stack segment)
    push eax             ; ESP (user stack pointer)
    pushf                ; EFLAGS — start from current flags …
    pop eax
    or  eax, EFLAGS_IF   ; … but guarantee IF=1
    push eax
    push USER_CODE_SEL   ; CS  (user code segment, RPL=3)
    push ecx             ; EIP (user entry point)

    iret                 ; off to Ring 3 — this does not return
