bits 32

global context_switch
global scheduler_enter_idle

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

; ============================================================================
; void scheduler_enter_idle(uintptr_t idle_sp)
;
; One-way transition from the bootstrap execution context to idle_thread.
;
; This is NOT a normal context switch:
;   - the bootstrap stack is not saved
;   - idle_thread.kernel_sp is not modified
;   - execution never returns
;
; idle_sp points to:
;
;     EDI
;     ESI
;     EBX
;     EBP
;     return address
; ============================================================================

scheduler_enter_idle:

    mov esp, [esp + 4]

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
