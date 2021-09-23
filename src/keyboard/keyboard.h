#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

typedef int32_t (*KEYBOARD_INIT_FUNCTION)();

struct process;
struct keyboard //Every keyboard is mapped to this struct 
{
    KEYBOARD_INIT_FUNCTION init; //Initialized the driver of the keyboard
    char name[20]; //Name of the keyboard
    struct keyboard* next; //Next keyboard on the list (linked list)
};

void keyboard_init();
void keyboard_backspace(struct process* process);
void keyboard_push(char c);
int32_t keyboard_insert(struct keyboard* keyboard);
char keyboard_pop();
#endif