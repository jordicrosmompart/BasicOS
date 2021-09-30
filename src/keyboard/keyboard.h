#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>
#define KEYBOARD_CAPS_LOCK_ON 1
#define KEYBOARD_CAPS_LOCK_OFF 0

typedef int KEYBOARD_CAPS_LOCK_STATE;

typedef int32_t (*KEYBOARD_INIT_FUNCTION)();

struct process;
struct keyboard //Every keyboard is mapped to this struct 
{
    KEYBOARD_INIT_FUNCTION init; //Initialized the driver of the keyboard
    char name[20]; //Name of the keyboard
    KEYBOARD_CAPS_LOCK_STATE caps_lock_state;
    struct keyboard* next; //Next keyboard on the list (linked list)
};

void keyboard_init();
void keyboard_backspace(struct process* process);
void keyboard_push(char c);
int32_t keyboard_insert(struct keyboard* keyboard);
char keyboard_pop();
void keyboard_set_caps_lock(struct keyboard* keyboard, KEYBOARD_CAPS_LOCK_STATE state);
KEYBOARD_CAPS_LOCK_STATE keyboard_get_caps_lock(struct keyboard* keyboard);
#endif