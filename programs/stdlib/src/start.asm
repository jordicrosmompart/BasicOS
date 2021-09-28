[BITS 32]

global _start
extern main

section .asm

;Entry point required for the linker
_start:
    call main
    ret