section .asm
global gdt_load

gdt_load:
    mov eax, [esp+4] ; Start address of the GDT descriptor is at esp+4 (esp+0 is return addres, esp+8 is size)
    mov [gdt_descriptor +2], eax ; Load the start addres to gdt_descriptor
    mov ax, [esp+8] ; Size of GDT descriptor passed to ax
    mov [gdt_descriptor], ax ; Load the size
    lgdt[gdt_descriptor]
    ret

section .data
gdt_descriptor:
    dw 0x00 ; Size
    dd 0x00 ; GDT Start Address