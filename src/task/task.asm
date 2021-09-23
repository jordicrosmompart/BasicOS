[BITS 32]

section .asm

global restore_general_purpose_registers
global task_return
global user_registers

task_return: ; using registers* struct. Enter user land. Faking an interrupt. We are pushing to the stack all what the processor would push when an interrupt happens
    mov ebp, esp
    ;Push the data segment
    ;Push the stack address
    ;Push the flags
    ;Push the code segment
    ;Push the IP

    ;Access structure passed to us, the registers. It is the offset 4 because the offset 0 is the return address
    mov ebx, [ebp+4] 
    ;Push the data/stack selector
    push dword [ebx+44] ;SS
    ;Push the stack pointer
    push dword [ebx+40] ; esp stack pointer

    ;Push the flags
    pushf
    pop eax
    or eax, 0x200 ; Manually reactivate interrupts, that is what cli/sli manages
    push eax ; Push back the flags with the interrupts reenable set :)

    ; Push the code segment
    push dword [ebx+32] ; CS 

    ; Push the IP to execute
    push dword [ebx+28] ; IP

    ;Above we have all what a normal interrupt would do, push all these registers!!

    ; Setup some segment register
    mov ax, [ebx+44] ; Set all the registers to the stack segment (set to 0x23, USER_DATA_SEGMENT ring level 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [ebp+4] ; Push registers to stack, to set them
    call restore_general_purpose_registers ; Set the registers passed, to not lose data from user land process
    add esp, 4 ; Undo the push to leave the stack clean to perform the iret

    ;Lets leave kernel land and execute in user land
    ;The processor will act as if it is returning from an interrupt, popping all the registers pushed in the stack, dropping to user land
    iretd

restore_general_purpose_registers: ; using registers* struct, to load another task register or kernel registers
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8] ; Get function parameter, a struct of registers
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12]
    pop ebp
    ret

user_registers: ; Sets the register back to the offset of the GDT entry for the USER_DATA_SEGMENT
    mov ax, 0x23 ; Offset 20 + ring privilege (ring 3) (see more about Request Protection Level)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret