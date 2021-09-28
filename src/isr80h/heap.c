#include "heap.h"
#include "task/task.h"
#include "task/process.h"
#include <stddef.h>
#include <stdint.h>

//Allocates memory for the current process
void* isr80h_command4_malloc(struct interrupt_frame* frame)
{
    size_t size = (int32_t) task_get_stack_item(task_current(), 0); //Get size
    return process_malloc(task_current()->process, size); //Allocate it
}

//Frees allocated memory for the current process
void* isr80h_command5_free(struct interrupt_frame* frame)
{
    void* ptr_to_free = task_get_stack_item(task_current(), 0); //Get pointer to free
    process_free(task_current()->process, ptr_to_free); //Free it
}