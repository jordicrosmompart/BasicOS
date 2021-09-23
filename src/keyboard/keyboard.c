#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"
#include "classic.h"


static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;


//Initialized precompiled keyboard drivers
void keyboard_init()
{
     keyboard_insert(classic_init()); //We only have the classic keyboard
}

//Add a keyboard
int32_t keyboard_insert(struct keyboard* keyboard)
{
    int32_t res = 0;
    if(keyboard->init == 0) //The new keyboard must have an initialization function
    {
        res = -EINVARG;
        goto out;
    }

    if(keyboard_list_last) //If there is a last keyboard (a keyboard)
    {
        keyboard_list_last->next = keyboard; //The next item is the new keyboard
        keyboard_list_last = keyboard; //The new keyboard will be the new last keyboard
    }
    else //No keyboard
    {
        keyboard_list_head = keyboard; //New keyboard will be the first
        keyboard_list_last = keyboard; //And the last
    }

    res = keyboard->init(); //Initialize keyboard after adding it to the linked list

out:
    return res;
}

static int32_t keyboard_get_tail_index(struct process* process)
{
    return process->keyboard.tail % sizeof(process->keyboard.buffer); //Returns the value of the tail % the size of the buffer of the keyboard, to not overflow the buffer
}

void keyboard_backspace(struct process* process) //Backspace means erase the last input character
{
    process->keyboard.tail -= 1; //Decrease the tail
    int32_t real_index = keyboard_get_tail_index(process); //Get used index
    process->keyboard.buffer[real_index] = 0x00; //Assign it to null
}
void keyboard_push(char c)
{
    struct process* process = process_current(); //Get current process
    if(!process)
    {
        return;
    }

    if(c == 0x00) //null characters are not inserted
    {
        return;
    }
    
    int32_t real_index = keyboard_get_tail_index(process); //Get tail index
    process->keyboard.buffer[real_index] = c; //Write to the buffer
    process->keyboard.tail++; //Increment tail index
}

char keyboard_pop()
{
    if(!task_current())
    {
        return 0;
    }

    struct process* process = task_current()->process;
    int32_t real_index = process->keyboard.head % sizeof(process->keyboard.buffer); //Get head
    char c = process->keyboard.buffer[real_index]; //Get character
    if(c == 0x00)
    {
        return 0; //Nothing to pop
    }
    process->keyboard.buffer[real_index] = 0; //Character read, we can nullify it now
    process->keyboard.head++; //increment the head
    return c;
}