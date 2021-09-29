#include "process.h"
#include "task/task.h"
#include "config.h"
#include "status.h"
#include "string/string.h"
#include "task/process.h"

//Loads a process from the filename provided at the stack
void* isr80h_command6_process_load_start(struct interrupt_frame* frame)
{
    void* filename_user_ptr = task_get_stack_item(task_current(), 0); //Gets pointer to the filename
    char filename[CROSOS_MAX_PATH];
    int res = copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename)); //Get copy the string to 'filename'
    if(res < 0)
    {
        goto out;
    }
    char path[CROSOS_MAX_PATH];
    strcpy(path, "0:/"); //Add the root directory to the path. It is a fast workaround
    strcpy(path+3, filename); //Concat the filename gotten from the stack
    struct process* process = 0;
    res = process_load_switch(path, &process); //Load the process
    if(res < 0)
    {
        goto out;
    }

    task_switch(process->task); //Switch to the task (page directory and 'current_task')
    task_return(&process->task->registers); //Set the registers to the new task registers

out:
    return 0;
}