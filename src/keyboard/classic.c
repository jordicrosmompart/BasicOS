#include "classic.h"
#include "keyboard.h"
#include "io/io.h"
#include "kernel.h"
#include "idt/idt.h"
#include "task/task.h"
#include <stdint.h>
#include <stddef.h>

#define CLASSIC_KEYBOARD_CAPS_LOCK 0x3A

int32_t classic_keyboard_init();

//Every index of the array below is a scancode that corresponds to the character that is containing
//Every keyboard type has a different scancode, thats why a Spanish keyboard will not map the same keys than the array below
static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`',
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

struct keyboard classic_keyboard = {
    .name = {"Classic"},
    .init = classic_keyboard_init
}; //Struct for the classic keyboard, providing a name and the initialization function.

void classic_keyboard_handle_interrupt();

//The following initialization function sets an interrupt callback for the code 0x21, the one set for the keyboard.
//Commonly the keyboard interrupt is the 0x01, but we have offset all the interrupts by 20 to have give space for exceptions and other standard interrupts of the processor
//See kernel.asm:25
int32_t classic_keyboard_init()
{
    //Registers the interrupt 0x21, keyboard
    idt_register_interrupt_callback(ISR_KEYBOARD_INTERRUPT, classic_keyboard_handle_interrupt);
    keyboard_set_caps_lock(&classic_keyboard, KEYBOARD_CAPS_LOCK_OFF);
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT); //Port 0x64, 0xAE enables the first PS2 port
    return 0;
}

//Gets a scancode and returns a char, caps wise
uint8_t classic_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    if(scancode > size_of_keyboard_set_one) //The scancode cannot be higher the number of scancodes we have in the keyboard
    {
        return 0;
    }

    if(scancode == CLASSIC_KEYBOARD_CAPS_LOCK)
    {
        KEYBOARD_CAPS_LOCK_STATE old_state = keyboard_get_caps_lock(&classic_keyboard);
        keyboard_set_caps_lock(&classic_keyboard, old_state == KEYBOARD_CAPS_LOCK_ON ? KEYBOARD_CAPS_LOCK_OFF : KEYBOARD_CAPS_LOCK_ON);
    }
    char c = keyboard_scan_set_one[scancode]; //Return the index
    if(keyboard_get_caps_lock(&classic_keyboard) == KEYBOARD_CAPS_LOCK_OFF)
    {
        if(c >= 'A' && c <= 'Z')
        {
            c += 32; 
        }
    }
    return c;
}

//Handler for a keyboard interrupt
void classic_keyboard_handle_interrupt()
{
    kernel_page(); //Switch to kernel page directory
    uint8_t scancode = 0;
    scancode = insb(KEYBOARD_INPUT_PORT); //Scan the code from the input port
    insb(KEYBOARD_INPUT_PORT); //See PC2 driver for more info.

    if(scancode & CLASSIC_KEYBOARD_KEY_RELEASED)
    {
        //Key released, TODO
        return;
    }

    uint8_t c = classic_keyboard_scancode_to_char(scancode); //Get the char from the scancode
    if(c != 0)
    {
        keyboard_push(c); //Push it to the process' keyboard buffer
    }

    task_page(); //Go back to user page directory

}

//Classic keyboard getter
struct keyboard* classic_init()
{
    return &classic_keyboard;
}