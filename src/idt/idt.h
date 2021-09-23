#ifndef IDT_H
#define IDT_H

#include <stdint.h>
struct interrupt_frame;
typedef void*(*ISR80H_COMMAND) (struct interrupt_frame* frame);
typedef void (*INTERRUPT_CALLBACK_FUNCTION)();

struct idt_desc 
{
    uint16_t offset_1; //Offset bits 0-15
    uint16_t selector; // Selector that is in out GDT
    uint8_t zero; // unused bits
    uint8_t type_attr; //Descriptor type and attributes
    uint16_t offset_2; //Offset bits 16-31
} __attribute__((packed));

struct idtr_desc
{
    uint16_t limit; //Size of IDT -1
    uint32_t base; //Base address of IDT
} __attribute__((packed));

struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved; //ESP Stack pointer that the pushad puts to the stack
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    //Values up are pushed by 'pushad'
    //Values down are pushed by the processor when an interrupt happens
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp; //Userland stack pointer
    uint32_t ss;
}__attribute__((packed));

void idt_init();
void enable_interrupts();
void disable_interrupts();
void isr80h_register_command(int32_t command_id, ISR80H_COMMAND command);
int32_t idt_register_interrupt_callback(int32_t interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback);
#endif