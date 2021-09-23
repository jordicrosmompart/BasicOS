#include "io.h"
#include "task/task.h"
#include "kernel.h"
#include "keyboard/keyboard.h"

// Prints a message pushed on the stack
void* isr80h_command1_print(struct interrupt_frame* frame)
{
    void* user_space_msg_buff = task_get_stack_item(task_current(), 0); //Get pointer to the message
    char buff[1024];
    copy_string_from_task(task_current(), user_space_msg_buff, buff, sizeof(buff)); //Copy from virtual address
    
    print(buff); //Print the message
    return 0;
}

//Gets a key from the keyboard (already stored from a typing)
void* isr80h_command2_getkey(struct interrupt_frame* frame)
{
    char c = keyboard_pop(); //Pops a character from the current process' keyboard
    return (void*)((int32_t) c);
}

//Prints a key that is pushed to the stack
void* isr80h_command3_putchar(struct interrupt_frame* frame)
{
    char c = (char)(int) task_get_stack_item(task_current(), 0); //Gets the character from the stack
    terminal_writechar(c, 15); //Prints it
    return 0;
}