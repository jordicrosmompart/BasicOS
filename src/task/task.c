#include "task.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "process.h"
#include "idt/idt.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "loader/formats/elfloader.h"

//Current task that is running
struct task* current_task = 0;

//Task linked list
struct task* task_tail = 0;
struct task* task_head = 0;

int32_t task_switch(struct task* task)
{
    current_task = task;
    paging_switch(task->page_directory); //Set the paging directory to the one of the current task
    return 0;
}

void task_save_state(struct task* task, struct interrupt_frame* frame)
{
    //Save all the registers that come from the frame of the interrupt
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}

//The 'virtual' addres cannot be accessed from kernel land directly
//Solution
int32_t copy_string_from_task(struct task* task, void* virtual, void* phys, int32_t max)
{
    int32_t res = 0;
    if(max >= PAGING_PAGE_SIZE)
    {
        res = -EINVARG;
        goto out;
    }

    
    char* tmp = kzalloc(max); //Allocate some memory in the kernel land (linearly paged) that will be shared with the task memory
    if(!tmp)
    {
        res = -ENOMEM;
        goto out;
    }

    uint32_t* task_directory = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_directory, tmp); //Get paging entry from the task for the address allocated in the kernel land
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL); //We map the physical address of the tmp (created in kernel mode) to the same virtual address number for the task paging directory
    //That allowed the task to see the memory created in the kernel,  'tmp'
    paging_switch(task->page_directory);  //Switch to the task page
    strncpy(tmp, virtual, max); //Copy from virtual task address to tmp, since virtual contains the string in the task paging and tmp points to the same physical address than kernel paging
    kernel_page(); //Switch again to kernel paging, since the tmp is already placed where the kernel knows

    res = paging_set(task_directory, tmp, old_entry); //Restore the virtual address modified 'tmp' to the old paging, to recover the original paging before copying the string
    if(res < 0)
    {
        res = -EIO;
        goto out_free;
    }

    strncpy(phys, tmp, max); //Copy from 'tmp' (that points to the string we want) to the 'phys'

out_free:
    kfree(tmp); //Free tmp because all contents are already copied to 'phys'

out:
    return res;
}

void task_current_save_state(struct interrupt_frame* frame)
{
    if(!task_current())
    {
        panic("No current task to save");
    }

    struct task* task = task_current(); //Get current task
    task_save_state(task, frame); //Save its state
}
int32_t task_page()
{
    user_registers(); //Sets the registers to the GDT USER_DATA_SEGMENT offset
    task_switch(current_task); // Switch the task to the current task, leaving the kernel interrupt
    return 0;
}

int32_t task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

void task_run_first_ever_task()
{
    if(!current_task)
    {
        panic("task_run_first_ever_task(): No current task existent");
    }
    task_switch(task_head); //Sets the current task and the loads its page directory
    task_return(&task_head->registers); // Performs the fake interrupt return and enters the process in user mode
}

uint32_t task_init(struct task* task, struct process* process);

struct task* task_current() //Gets current task
{
    return current_task;
}

struct task* task_new(struct process* process)
{
    uint32_t res = 0;
    struct task* task = kzalloc(sizeof(struct task)); //Creates and allocates a new task
    if(!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if(res != CROSOS_ALL_OK)
    {
        goto out;
    }

    if(task_head == 0) //If we dont have a task_head (creating first task)
    {
        task_head = task; //The task is the head
        task_tail = task; //And the tail
        current_task = task;
        goto out;
    }

    task_tail->next = task; //Link tail with a next. 
    task->prev = task_tail; // Set the previous 'task_tail' to the previous task of the new one
    task_tail = task; // Set the 'task_tail' to the new task

out:
    if(ISERR(res))
    {
        task_free(task); //Free the task if there has been an error
        return ERROR(res);
    }
    return task;
}

struct task* task_get_next() //It can return null if there are no more tasks
{
    if(!current_task->next)
    {
        return task_head; //Return first task if the current task is the last one (tail_task)
    }

    return current_task->next;
}

static void task_list_remove(struct task* task)
{
    if(task->prev)
    {
        task->prev->next = task->next; //Link the previous task with the next task
    }

    if(task == task_head)
    {
        task_head = task->next; //If it is the first of the list, the following one will be now
    }

    if(task == task_tail)
    {
        task_tail = task->prev; //If it is the last of the list, the previous one will be now
    }

    if(task == current_task)
    {
        current_task = task_get_next(); //Set the next as a current task
    }
}
uint32_t task_free(struct task* task)
{
    paging_free_4gb(task->page_directory); //Free the paging directory for the task
    task_list_remove(task); //Remove from the list

    kfree(task); //Free the task data

    return 0;
}

uint32_t task_init(struct task* task, struct process* process)
{
    memset(task, 0x00, sizeof(struct task)); //Initialize the task

    //Map the entire 4GB address space to itself
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL); //Create a new paging directory for the task

    if(!task->page_directory)
    {
        return -EIO;
    }
    
    task->registers.ip = CROSOS_PROGRAM_VIRTUAL_ADDRESS; //When we first create a task, the start address is a hardcoded value
    if(process->filetype == PROCESS_FILETYPE_ELF)
    {
        task->registers.ip = elf_header(process->elf_file)->e_entry;
    }
    task->registers.ss = USER_DATA_SEGMENT; //The created tasks are used by users
    task->registers.cs = USER_CODE_SEGMENT; //The CS register must point to the user's code segment
    task->registers.esp = CROSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START; //Stack address is shared virtually for all tasks. Note that the physical addres will be different, since all different tasks have different paging directories
    task->process = process; // Process of the task must be referenced
    return 0;

}

//Run the function only in kernel page
void* task_get_stack_item(struct task* task, int32_t index)
{
    void* result = 0;
    uint32_t* sp_ptr = (uint32_t*) task->registers.esp; //Get stack pinter saved when we entered in the interrupt
    
    //Switch to provided task's page
    task_page_task(task);

    result = (void*) sp_ptr[index]; //Get the element 'index' on the stack pointer of the task
    //Switch back to kernel page
    kernel_page();
    //Kernel ready to continue with the interrupt

    return result;
}