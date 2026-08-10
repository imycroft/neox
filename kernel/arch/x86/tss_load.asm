bits 32

global tss_load

section .text

;-------------------------------------------------------
; void tss_load(uint16_t selector)
;
; Loads the Task Register with the TSS selector so
; the CPU can find the TSS on privilege-level changes.
;-------------------------------------------------------

tss_load:
    mov ax, [esp + 4]
    ltr ax
    ret
