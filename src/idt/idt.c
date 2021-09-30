#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "task/task.h"
#include "status.h"
#include "task/process.h"

struct idt_desc idt_descriptors[CROSOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void* interrupt_pointer_table[CROSOS_TOTAL_INTERRUPTS];

static ISR80H_COMMAND isr80h_commands[CROSOS_MAX_ISR80H_COMMANDS];
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[CROSOS_TOTAL_INTERRUPTS];

//Calls to ASM code
extern void idt_load(struct idtr_desc* ptr);
extern void int21h();
extern void no_interrupt();
extern void isr80h_wrapper();

//No interrupt implemented
void no_interrupt_handler()
{
    outb(0x20, 0x20); // ACK sent to successfully release the interrupt
}

//This function is called when an interrupt happens
void interrupt_handler(int32_t interrupt, struct interrupt_frame* frame)
{
    kernel_page();
    if(interrupt_callbacks[interrupt] != 0)
    {
        task_current_save_state(frame);
        interrupt_callbacks[interrupt](frame); //Call the interrupt function number, stored in the array
    }
    task_page();
    outb(0x20, 0x20); // ACK sent to successfully release the interrupt
}

//Interrupt divide by zero handler
void idt_zero() 
{
    print("Divide by zero error\n"); //Needs to use iret and the outb(0x20, 0x20)...
}

//Sets an entry to the idt
void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t) address & 0x0000ffff; //first 16 bits of the address
    desc->selector = KERNEL_CODE_SELECTOR; // using the code selector of the GDT
    desc->zero = 0x00; // unused bits
    desc->type_attr = 0xEE; //type of gate type + other flags. Check OSDev IDT for more
    desc->offset_2 = (uint32_t) address >> 16; //last 16 bits of the address of the interrupt
}

//Interrupt handler for all exceptions
void idt_handle_exception()
{
    process_terminate(task_current()->process); //Terminate the process

    task_next(); //Switch to next task
}

//Interrupt handler for a clock tick
void idt_clock()
{
    outb(0x20, 0x20); // ACK sent to successfully release the interrupt
    task_next(); //Switch to the next task
}

//Initializes the IDT
void idt_init() 
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors)); // Initialize the IDT to null to set them in memory
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1; // Size of the allocated IDT
    idtr_descriptor.base = (uint32_t) idt_descriptors; // Address of the allocated IDT

    for(int i=0; i < CROSOS_TOTAL_INTERRUPTS; i++) 
    {
        idt_set(i, interrupt_pointer_table[i]); // Set the no_interrupt for every entry on the table, so all of them, even they are not handled, they acknowledge and iret correctly
    }
    idt_set(0, idt_zero); // Set the Interrupt 0. It does not use the iret, so it is a bad design even though it works
    idt_set(0x80, isr80h_wrapper); //User land interrupts

    for(int i = 0; i < 20; i++)
    {
        idt_register_interrupt_callback(i, idt_handle_exception);
    }

    idt_register_interrupt_callback(0x20, idt_clock); //Register the clock interrupt

    //Load the interrupt descriptor table
    idt_load(&idtr_descriptor); // Call asm instruction in idt.asm
}

//Sets an interrupt handler to the interrupts array position passed as 'interrupt'
int32_t idt_register_interrupt_callback(int32_t interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    if(interrupt < 0 || interrupt >= CROSOS_TOTAL_INTERRUPTS)
    {
        return -EINVARG;
    }
    interrupt_callbacks[interrupt] = interrupt_callback;
    return CROSOS_ALL_OK;
}

//Registers a pointer to a function that handles a user interrupt (0x80)
void isr80h_register_command(int32_t command_id, ISR80H_COMMAND command)
{
    if(command_id < 0 || command_id >= CROSOS_MAX_ISR80H_COMMANDS)
    {
        panic("The command is out of bounds\n");
    }

    if(isr80h_commands[command_id])
    {
        panic("Trying to overwrite an existing command\n");
    }

    isr80h_commands[command_id] = command;
}

//It handles all user interrupts (0x80)
void* isr80h_handle_command(int32_t command, struct interrupt_frame* frame)
{
    void* result = 0;

    if(command < 0 || command >= CROSOS_MAX_ISR80H_COMMANDS)
    {
        return 0; //Invalid command
    }

    ISR80H_COMMAND command_func = isr80h_commands[command]; //Gets the function pointer stored at initialization time
    if(!command_func)
    {
        return 0;
    }

    result = command_func(frame); //Call to the function that handles the command
    return result;
}

//Called by interrupt 0x80 wrapper in ASM
void* isr80h_handler(uint32_t command, struct interrupt_frame* frame) 
{
    void* res = 0;
    kernel_page(); //Activates the kernel page directory and segment registers of the GDT
    task_current_save_state(frame); //Save registers of the task
    res = isr80h_handle_command(command, frame); //Handles the command of the interrupt
    task_page(); //Activates again the user task page directory
    return res; //Return result from interrupt command function
}