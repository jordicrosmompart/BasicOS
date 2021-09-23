section .asm

extern int21h_handler
extern interrupt_handler
extern no_interrupt_handler
extern isr80h_handler

global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp ; Push base pointer to stack
    mov ebp, esp ; Change scope of the base pointer to stack pointer
    mov ebx, [ebp+8] ; Move first argument passed to the function
    lidt [ebx] ; Set IDT
    pop ebp ; Recover base pointer from stack
    ret

no_interrupt:
    pushad
    call no_interrupt_handler ; Void function for the interrupts without a handle
    popad
    iret ; NEEDED, thats the reason for this no_interrupt to exist, apart from the ACK

;Macro that creates a function for every interrupt (512). It prevents the coder to code 512 functions that are the same
%macro interrupt 1
    global int%1
    int%1:
        pushad
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro

%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep


isr80h_wrapper:
    ;Interrupt frame start
    ;Already pushed to us by the processor upon entry of the interrupt
    ;uint32_t ip
    ;uint32_t cs
    ;uint32_t flags
    ;uint32_t sp
    ;uint32_t ss

    pushad ; Push general purpose registers to stack
    ; EDI, ESI, EBP, ESP, EDX, ECX, EBX, EAX

    ; Interrupt frame end

    ;Push stack pointer so that we are pointing to the interrupt frame, that contains all the pushed registers
    ; By this, we can cast all the information of the interrupt frame, by knowing the position of this stack pointer
    push esp

    push eax ; Contains the command that the kernel should invoke, push it to the stack for the handler
    call isr80h_handler
    mov dword[tmp_res], eax ; return value of the eax is stored at a tmp_res memory location
    add esp, 8 ; Returns stack pointer as if the previous two elements weren't pushed

    ;Restore general purpose registers for user land
    popad
    mov eax, [tmp_res] ; Move back the tmp_res value to eax, the return value for C functions
    iretd

section .data
;Stores the temporary return result from the isr80h_handler
tmp_res: dd 0

%macro interrupt_array_entry 1
    dd int%1 ; Gets the addres of every function created by the above macro
%endmacro

interrupt_pointer_table: ; Creates an array of pointers to the interrupt functions
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep