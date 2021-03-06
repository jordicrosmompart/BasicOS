#include "process.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "memory/paging/paging.h"
#include "loader/formats/elfloader.h"

struct process* current_process = 0; //Current process that is running

static struct process* processes[CROSOS_MAX_PROCESSES] = {};

//Cleans the allocated memory for the process
static void process_init(struct process* process)
{
    memset(process, 0x00, sizeof(struct process)); 
}

//Returns current process
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

//Sets the parameter process to the current process
int32_t process_switch(struct process* process)
{
    current_process = process;
    return 0;
}

//Finds a free spot on the allocations array
static int32_t process_find_free_allocation_index(struct process* process)
{
    int32_t res = -ENOMEM;
    for(int32_t i = 0; i<CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == 0)
        {
            res = i; // Return the free index
            break;
        }
    }
    return res;
}

//Allocates memory for a process and stores it on its allocations array
void* process_malloc(struct process* process, size_t size)
{
    void* ptr = kzalloc(size); //Allocate the memory
    if(!ptr)
    {
        return 0;
    }
    int32_t index = process_find_free_allocation_index(process); //Find a free index on the array
    if(index < 0)
    {
        goto out_err;
    }
    int res = paging_map_to(process->task->page_directory, ptr, ptr, paging_align_address(ptr+size), PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL); //Creates a new page for the malloc
    if(res < 0)
    {
        goto out_err;
    }
    process->allocations[index].ptr = ptr; //Assign the free spot to the allocated array
    process->allocations[index].size = size;
    return ptr;

out_err:
    if(ptr)
    {
        kfree(ptr);
    }
    return 0;
}

//Finds for the pointer in the allocations array of a process
static bool process_is_process_pointer(struct process* process, void* ptr)
{
    for(int32_t i = 0; i < CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == ptr)
        {
            return true; //Pointer found
        }
    }
    return false;
}

//Frees the pointer found in the process' allocations array
static void process_allocation_free(struct process* process, void* ptr)
{
    for (int32_t i = 0; i < CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == ptr)
        {
            process->allocations[i].ptr = 0x00; //Free the allocation entry
            process->allocations[i].size = 0;
        }
    }
}

//Returns the allocation that matches with the provided pointer
static struct process_allocation* process_get_allocation_by_addr(void* addr, struct process* process)
{
    for(int i = 0; i < CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(process->allocations[i].ptr == addr)
        {
            return &process->allocations[i];
        }
    }
    return 0;
}

//Returns the arguments of the proces
void process_get_arguments(struct process* process, int* argc, char*** argv)
{
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

//Counts the number of arguments linked to the parameter structure
int process_count_command_arguments(struct command_argument* root_argument)
{
    struct command_argument* current = root_argument;
    int i = 0;
    while(current)
    {
        i++;
        current = current->next;
    }
    return i;
}

//Loads arguments to the current process
int process_inject_arguments(struct process* process, struct command_argument* root_argument)
{
    int res = 0;

    struct command_argument* current = root_argument;
    int i = 0;
    int argc = process_count_command_arguments(root_argument);
    if(!argc)
    {
        res = -EIO;
        goto out;
    }

    char** argv = process_malloc(process, sizeof(const char*) * argc);
    if(!argv)
    {
        res = -ENOMEM;
        goto out;
    }

    while(current)
    {
        char* argument_str = process_malloc(process, sizeof(current->argument));
        if(!argument_str)
        {
            res = -ENOMEM;
            goto out;
        }

        strncpy(argument_str, current->argument, sizeof(current->argument));
        argv[i] = argument_str;
        current = current->next;
        i++;
    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;

out:
    return res;
}

//Frees the allocations of a process
static int process_terminate_allocations(struct process* process)
 {
     for(int i = 0; i < CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
     {
         process_free(process, process->allocations[i].ptr);
     }

     return 0;
 }

//Frees the loaded binary data of a process
static int process_free_binary_data(struct process* process)
{
    kfree(process->ptr);
    return 0;
}

//Frees the loaded data of an elf file
static int process_free_elf_data(struct process* process)
{
    elf_close(process->elf_file);
    return 0;
}

//Frees the memory of a process
static int process_free_program_data(struct process* process)
{
    int res = 0;
    switch(process->filetype)
    {
        case PROCESS_FILETYPE_BINARY:
            res = process_free_binary_data(process);
            break;
        case PROCESS_FILETYPE_ELF:
            res = process_free_elf_data(process);
            break;
        default:
            res = -EINVARG;
    }
    return res;
}

//Switches the process to the first process found
static void process_switch_to_any()
{
    for(int i = 0; i < CROSOS_MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if(processes[i])
        {
            process_switch(processes[i]);
            return;
        }
    }

    panic("No processes to switch to");
}

//Unlinks a process from the linked list
static void process_unlink(struct process* process)
{
    processes[process->id] = 0x00;
    if(current_process == process)
    {
        process_switch_to_any();
    }
}

//Terminates a process
int process_terminate(struct process* process)
{
    int res = 0;
    res = process_terminate_allocations(process);
    if(res < 0)
    {
        goto out;
    }

    res = process_free_program_data(process);
    if(res < 0)
    {
        goto out;
    }
    kfree(process->stack); //Free the process stack memory

    res = task_free(process->task);
    if(res < 0)
    {
        goto out;
    }

    process_unlink(process);

out:
    return res;
}

//Frees the allocation array entry for a pointer and its contents
void process_free(struct process* process, void* ptr)
{

    struct process_allocation* allocation = process_get_allocation_by_addr(ptr, process);//Unlink the pages from the process for the given address
    
    if(!allocation)
    {
        return; //Not our pointer
    }

    int res = paging_map_to(process->task->page_directory, allocation->ptr, allocation->ptr, paging_align_address(allocation->ptr + allocation->size), 0x00); //Free the page used before

    if(res < 0)
    {
        return;
    }

    process_allocation_free(process, ptr); //Free allocation
    kfree(ptr); //Free contents in the pointer

}

//Loads the process binary file
static int32_t process_load_binary(const char* filename, struct process* process)
{
    void* program_data_pointer = 0x00;
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

    program_data_pointer = kzalloc(stat.filesize); //Allocates the size of the process binary file to memory
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
    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_pointer; //Stores physical location
    process->size = stat.filesize; //And size

out:
    if(res < 0)
    {
        if(program_data_pointer)
        {
            kfree(program_data_pointer);
        }
    }
    fclose(fd); //Since it is loaded into memory, we dont need the file handle anymore
    return res;
}

//Loads an elf file process
static int32_t process_load_elf(const char* filename, struct process* process)
{
    int32_t res = 0;
    struct elf_file* elf_file = 0;
    res = elf_load(filename, &elf_file); //Load elf file and structure into memory
    if (ISERR(res))
    {
        goto out;
    }
    process->filetype = PROCESS_FILETYPE_ELF; //Map the filetype
    process->elf_file = elf_file; //And pointer to created struct

out:   
    return res;
}
//If we ever use elf files, this function is a generic access for different process formats
static int32_t process_load_data(const char* filename, struct process* process)
{
    int32_t res = 0;
    res = process_load_elf(filename, process); //Try to load elf
    if( res == -EINFORMAT) //If its not possible, try to load binary
    {
        res = process_load_binary(filename, process);
    }

    return res;
}

//Maps a binary file that is loaded into memory to a virtual address
int32_t process_map_binary(struct process* process)
{
    int32_t res = 0;
    paging_map_to(process->task->page_directory, (void*) CROSOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITABLE);
    return res;
}

//Maps an elf file to the corresponding page of the process structure
int32_t process_map_elf(struct process* process)
{
    int res = 0;

    struct elf_file* elf_file = process->elf_file; //Gets the elf_file allocated address
    struct elf_header* header = elf_header(elf_file); //Gets the elf header
    struct elf32_phdr* phdrs = elf_pheader(header); //Gets the program headers
    for(int32_t i = 0; i < header->e_phnum; i++) //Iterate every program header entry
    {
        struct elf32_phdr* phdr = &phdrs[i]; //Get the entry
        void* phdr_phys_address = elf_phdr_phys_address(elf_file, phdr); //Get physical address of the program's header
        int32_t flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL; //Flags set by OS
        if(phdr->p_flags & PF_W) //If the writable flag of the program header is set
        {
            flags |= PAGING_IS_WRITABLE; //Add the writable OS flag for the paging
        }
        //Set a page for the code that the header points to
        //Get the lower page of the header's virtual address and physical address
        //Provide end physical address and flags
        res = paging_map_to(process->task->page_directory, paging_align_to_lower_page((void*) phdr->p_vaddr), paging_align_to_lower_page((void*) phdr_phys_address), paging_align_address(phdr_phys_address + phdr->p_memsz), flags);
        if(ISERR(res))
        {
            break;
        }
    }
    return res;
}

//Generic call for different process formats
int32_t process_map_memory(struct process* process)
{
    int32_t res = 0;
    switch(process->filetype)
    {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
            break;
        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
            break;
        default:
            panic("The process map is neither binary or elf");
    }
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

//Loads a new process and sets it as the current one
int32_t process_load_switch(const char* filename, struct process** process)
{
    int32_t res = process_load(filename, process);
    if(res == 0)
    {
        process_switch(*process);
    }

    return res;
}

//Loads a process from slot
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