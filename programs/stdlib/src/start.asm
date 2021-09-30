[BITS 32]

global _start
extern c_start
extern crosos_exit

section .asm

;Entry point required for the linker
_start:
    call c_start ; call to introduce parameters to main
    call crosos_exit
    ret