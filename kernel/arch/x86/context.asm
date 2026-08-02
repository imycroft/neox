bits 32

global context_switch

section .text

;-------------------------------------------------------
; void context_switch(uintptr_t *old_sp,
;                     uintptr_t new_sp)
;
; Stack on entry:
;
; [esp+4] = old_sp
; [esp+8] = new_sp
;-------------------------------------------------------

context_switch:

    ; Save callee-saved registers.
    push ebp
    push ebx
    push esi
    push edi

    ; Save current stack pointer.
    mov eax, [esp + 20]
    mov [eax], esp

    ; Load next thread stack.
    mov eax, [esp + 24]
    mov esp, eax

    ; Restore registers.
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
