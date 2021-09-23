[BITS 32]

section .asm

global _start
;Loop program, it gets a key and prints it
_start:
    
_loop:
    call getkey
    push eax
    mov eax, 3 ; Command 3:  putchar
    int 0x80
    add esp, 4 ; Recover original stack pointer, since we pushed message and we no longer need it
    
    jmp _loop ; Repeat process

getkey:
    mov eax, 2 ; Command 2: getkey
    int 0x80
    cmp eax, 0x00 ; Return next key on eax, if is not 0 keep getting key
    je getkey ; Loop until there is a key on the keyboard struct
    ret

section .data
message db 'User land talking with kernel',0x0a, 0