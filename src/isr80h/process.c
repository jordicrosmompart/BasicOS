#include "process.h"
#include "task/task.h"
#include "config.h"
#include "status.h"
#include "string/string.h"
#include "task/process.h"
#include "kernel.h"

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

//Calls a system command (process + argument)
void* isr80h_command7_invoke_system_command(struct interrupt_frame* frame)
{
    //Get arguments from stack
    struct command_argument* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    if(!arguments || strlen(arguments[0].argument) == 0)
    {
        return ERROR(-EINVARG);
    }

    struct command_argument* root_command_argument = &arguments[0];
    const char* program_name = root_command_argument->argument;

    char path[CROSOS_MAX_PATH];
    strcpy(path, "0:/");
    strncpy(path+3, program_name, sizeof(path));

    struct process* process = 0;
    int res = process_load_switch(path, &process); //Load and use the process
    if(res < 0)
    {
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument); //Inject the arguments to the process loaded
    if(res < 0)
    {
        return ERROR(res);
    }
    task_switch(process->task); //Switch back to user land
    task_return(&process->task->registers); //Set process' registers

    return 0;
}

//Gets the arguments of a process
void* isr80h_command8_get_program_arguments(struct interrupt_frame* frame)
{
    struct process* process = task_current()->process;
    struct process_arguments* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));

    process_get_arguments(process, &arguments->argc, &arguments->argv);
    return 0;
}

//Terminates a process and switches to the next one
void* isr80h_command9_exit(struct interrupt_frame* frame)
{
    struct process* process = task_current()->process;
    process_terminate(process);
    task_next();
    return 0;
}