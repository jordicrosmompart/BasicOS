[BITS 32]

section .asm

;This file is where the system calls are handled
;Set of global functions that perform an int 0x80 with the correct command number on the eax
global print:function
global crosos_getkey:function
global crosos_putchar:function
global crosos_malloc:function
global crosos_free:function
global crosos_process_load_start:function
global crosos_system:function
global crosos_process_get_arguments:function
global crosos_exit:function

; void print (const char* message)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8] ; offset 0 is the first parameter
    mov eax, 1 ; Cmd print
    int 0x80
    add esp, 4
    pop ebp
    ret

; int crosos_getey()
crosos_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2 ; Cmd getkey
    int 0x80
    pop ebp
    ret

; void crosos_putchar(char c)
crosos_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3 ; Cmd putchar
    push dword[ebp+8]
    int 0x80
    add esp, 4
    pop ebp
    ret

; void* croos_malloc(size_t size)
crosos_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4 ; Cmd malloc
    push dword[ebp+8] ; Parameter size
    int 0x80
    add esp, 4
    pop ebp
    ret

; void crosos_free(void* ptr)
crosos_free:
    push ebp
    mov ebp, esp
    mov eax, 5 ; Cmd free
    push dword[ebp+8] ;Pointer to free
    int 0x80
    add esp, 4
    pop ebp
    ret

; void crosos_process_load_start(const char* filename)
crosos_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, 6 ; Cmd start process
    push dword[ebp+8] ; Variable filename
    int 0x80
    add esp, 4
    pop ebp
    ret

; int crosos_system(struct command_argument* arguments)
crosos_system:
    push ebp
    mov ebp, esp
    mov eax, 7 ; Cmd system command
    push dword[ebp+8] ; Variable 'arguments'
    int 0x80
    add esp, 4
    pop ebp
    ret

; void crosos_process_get_arguments(struct process_arguments* arguments)
crosos_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, 8 ; Cmd get process arguments
    push dword[ebp+8] ; Variable arguments
    int 0x80
    add esp, 4
    pop ebp
    ret

; void crosos_exit()
crosos_exit:
    push ebp
    mov ebp, esp
    mov eax, 9 ; Cmd exit current process
    int 0x80
    pop ebp
    ret