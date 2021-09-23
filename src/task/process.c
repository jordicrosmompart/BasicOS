#include "process.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "memory/paging/paging.h"

struct process* current_process = 0; //Current process that is running

static struct process* processes[CROSOS_MAX_PROCESSES] = {};

static void process_init(struct process* process)
{
    memset(process, 0x00, sizeof(struct process)); //Cleans the allocated memory for the process
}

struct process* process_current()
{
    return current_process;
}

//Get process by id
struct process* process_get(uint32_t process_id)
{
    if(process_id < 0 || process_id >= CROSOS_MAX_PROCESSES)
    {
        return NULL;
    }

    return processes[process_id];
}

int32_t process_switch(struct process* process)
{
    current_process = process;
    return 0;
}

//Loads the process binary file
static int32_t process_load_binary(const char* filename, struct process* process)
{
    int32_t res = 0;
    uint32_t fd = fopen(filename, "r");
    if(!fd)
    {
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if(res != CROSOS_ALL_OK)
    {
        goto out;
    }

    void* program_data_pointer = kzalloc(stat.filesize); //Allocates the size of the process binary file to memory
    if(!program_data_pointer)
    {
        res = -ENOMEM;
        goto out;
    }

    if(fread(program_data_pointer, stat.filesize, 1, fd) != 1) //Reads the process and stores it to the process pointer already created
    {
        res = -EIO;
        goto out;
    }

    process->ptr = program_data_pointer; //Stores physical location
    process->size = stat.filesize; //And size

out:
    fclose(fd); //Since it is loaded into memory, we dont need the file handle anymore
    return res;
}

//If we ever use elf files, this function is a generic access for different process formats
static int32_t process_load_data(const char* filename, struct process* process)
{
    int32_t res = 0;
    res = process_load_binary(filename, process);
    return res;
}

//Maps a binary file that is loaded into memory to a virtual address
int32_t process_map_binary(struct process* process)
{
    int32_t res = 0;
    paging_map_to(process->task->page_directory, (void*) CROSOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITABLE);
    return res;
}

//If we ever use elf files, this function is a generic call for different process formats
int32_t process_map_memory(struct process* process)
{
    int32_t res = 0;
    res = process_map_binary(process);
    if(res < 0)
    {
        goto out;
    }

    paging_map_to(process->task->page_directory, (void*) CROSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack + CROSOS_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITABLE);

out:
    return res;
}

//Returns a free slot in the processes array
int32_t process_get_free_slot()
{
    for(int32_t i = 0; i < CROSOS_MAX_PROCESSES; i++)
    {
        if(processes[i] == 0)
        {
            return i;
        }
    }
    return -EISTKN;
}

//Loads a process
int32_t process_load(const char* filename, struct process** process)
{
    int32_t res = 0;
    int32_t process_slot = process_get_free_slot(); //Get a free slot
    if(process_slot < 0)
    {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);

out:
    return res;
}

int32_t process_load_switch(const char* filename, struct process** process)
{
    int32_t res = process_load(filename, process);
    if(res == 0)
    {
        process_switch(*process);
    }

    return res;
}

int32_t process_load_for_slot(const char* filename, struct process** process, uint32_t process_slot)
{
    int32_t res = 0;
    struct task* task = 0;
    struct process* _process;
    void* program_stack_pointer = 0;

    if(process_get(process_slot) != 0) //Checks that the slot is free
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process)); //Allocates memory for the new process struct
    if(!_process)
    {
        res = -ENOMEM;
        goto out;
    }

    process_init(_process); //Cleans the process memory allocation
    res = process_load_data(filename, _process); //Call to load the file from the filesystem to the memory
    if(res < 0)
    {
        goto out;
    }

    program_stack_pointer = kzalloc(CROSOS_USER_PROGRAM_STACK_SIZE); //Allocates a new stack for the process
    if(!program_stack_pointer)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename)); //Copies the filename to the new process struct
    _process->stack = program_stack_pointer;
    _process->id = process_slot;

    //Create a task
    task = task_new(_process); //Creates a task, and a page directory for it
    if(ERROR_I(task) == 0)
    {
        res = ERROR_I(task);
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process); //Maps the process address loaded into memory to a page
    if( res < 0)
    {
        goto out;
    }

    *process = _process; //Changes the pointer address of the provided process to the created one

    //Add process to array
    processes[process_slot] = _process;

out:
    if(ISERR(res))
    {
        if(_process && _process->task)
        {
            task_free(_process->task);
        }

    //Free process data
    }

    return res;
}