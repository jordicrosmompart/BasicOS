section .asm

global tss_load

tss_load: ; Loads in the TSS
    push ebp
    mov ebp, esp
    mov ax, [ebp+8] ; TSS offset segment provided (it would be +4 if the ebp wasnt pushed)
    ltr ax
    pop ebp
    ret