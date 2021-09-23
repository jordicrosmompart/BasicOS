#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"
struct interrupt_frame;
struct registers
{
    uint32_t edi;
    uint32_t esi; //Offset 4
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip; //Offset 28
    uint32_t cs; //Offset 32
    uint32_t flags;
    uint32_t esp; //Offset 40
    uint32_t ss; //Offset 44
};

struct process;
struct task
{
    //The page directory of the task
    struct paging_4gb_chunk* page_directory;

    //Registers of the task when the task is not running
    struct registers registers;

    //Next task in the linked list
    struct task* next;

    //The process of the task
    struct process* process;

    //Previous task in the linked list
    struct task* prev;
};

struct task* task_new(struct process* process);
struct task* task_current();
struct task* task_get_next();
uint32_t task_free(struct task* task);

int32_t task_switch(struct task* task);
int32_t task_page();

void task_run_first_ever_task();

extern void task_return(struct registers* registers);
extern void restore_general_purpose_registers(struct registers* registers);
extern void user_registers();

void task_current_save_state(struct interrupt_frame* frame);
int32_t copy_string_from_task(struct task* task, void* virtual, void* phys, int32_t max);
void* task_get_stack_item(struct task* task, int32_t index);
int32_t task_page_task(struct task* task);

#endif