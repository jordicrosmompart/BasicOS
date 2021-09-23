[BITS 32]

section .asm

global paging_load_directory
global enable_paging

paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8] ; copy paging directory address
    mov cr3, eax ; load it to cr3
    pop ebp
    ret

enable_paging:
    push ebp ; Save original ebp
    mov ebp, esp ;Base pointer is now the stack pointer (to not smash previous elements in the stack)
    mov eax, cr0
    or eax, 0x80000000; set bit that enables paging
    mov cr0, eax ; in the cr0 register
    pop ebp ; Recover original base pointer
    ret