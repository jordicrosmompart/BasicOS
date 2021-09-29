#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "config.h"
#include "task.h"
#include <stdbool.h>

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1

typedef unsigned char PROCESS_FILETYPE;

struct process_allocation
{
    void* ptr;
    size_t size;
};

struct process
{
    uint8_t id; //Process id
    char filename[CROSOS_MAX_PATH];
    struct task* task; //Main process task
    struct process_allocation allocations[CROSOS_MAX_PROGRAM_ALLOCATIONS]; //Whenever the process mallocs, we add the address here to free it when the process dies
    PROCESS_FILETYPE filetype; //It may be a binary or an elf file
    union 
    {
        void* ptr; //Physical pointer to process memory
        struct elf_file* elf_file;
    };
    void* stack; //Pointer to process stack
    uint32_t size; //Size of the data pointer by 'ptr'
    struct keyboard_buffer //Structure that holds the input buffer of the used keyboard
    //It doesnt hold a keyboard struct itself, it is only the buffer
    {
        char buffer[CROSOS_KEYBOARD_BUFFER_SIZE];
        int32_t tail;
        int32_t head;
    } keyboard;
};
int32_t process_switch(struct process* process);
int32_t process_load_switch(const char* filename, struct process** process);
int32_t process_load_for_slot(const char* filename, struct process** process, uint32_t process_slot);
int32_t process_load(const char* filename, struct process** process);
struct process* process_current();

void* process_malloc(struct process* process, size_t size);
void process_free(struct process* process, void* ptr);

#endif