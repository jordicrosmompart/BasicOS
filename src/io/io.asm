section .asm

global insb
global insw
global outb
global outw

insb:
    push ebp
    mov ebp, esp

    xor eax, eax ; Clear eax
    mov edx, [ebp+8] ; pass port parameter to edx 
    in al, dx ; get the value from dx port to al, the return value. EAX is always the return value for ASM/C

    pop ebp
    ret

insw:
    push ebp
    mov ebp, esp

    xor eax, eax ; Clear eax
    mov edx, [ebp+8] ; pass port parameter to edx
    in ax, dx ; get the value from dx port to al, the return value. EAX is always the return value for ASM/C

    pop ebp
    ret

outb:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12] ; Second parameter
    mov edx, [ebp+8] ; First parameter
    out dx, al

    pop ebp
    ret

outw:
    push ebp
    mov ebp, esp

    mov eax, [ebp+12] ; Second parameter
    mov edx, [ebp+8] ; First parameter
    out dx, ax

    pop ebp
    ret
