[BITS 32]
global _start
global kernel_registers
extern kernel_main


CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG ; set ds, es, fs, gs, ss to the data_segment offset in the GDT table (10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000 ; set the stack pointer (32 bits) to a higher address to have enough space
    mov esp, ebp

    ; Enable A20 line (fast mode) to have access to 21st bit of any memory address (compatibility reasons)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Remap the master Programmable Interrupt Controller. This could be done in C, but we only need to do it once
    mov al, 00010001b ; Init Master PIC code
    out 0x20, al ; Tell the master PIC

    mov al, 0x20 ; Interrupt 0x20 is where master ISR should start
    out 0x21, al ; Tell the master PIC

    mov al, 00000001b ; PIC in x86 mode
    out 0x21, al
    ;End remap of master PIC

    call kernel_main
    jmp $

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

times 512-($ - $$) db 0 ; Align 512 bytes
