#include "crosos.h"

//Waits for a key to be pressed
int crosos_getkeyblock()
{
    int val = 0;
    do
    {
        val = crosos_getkey(); //Interrupt routine for get key
    } while(!val);
}

//Reads a line from the terminal
void crosos_terminal_readline(char* out, int max, bool output_while_typing)
{
    int i = 0;
    for (i = 0; i< max-1; i++)
    {
        char key = crosos_getkeyblock();
        
        if(key == 13) //Check if key is 'Enter'
        {
            break;
        }

        if(output_while_typing) //Invisible typing or not
        {
            crosos_putchar(key); //Interrupt routine for put char
        }

        
        if(key == 0x08 && i >= 1) //Backspace
        {
            out[i-1] = 0x00; // Backspace the out array
            i -=2; //the iterator will be increased by 
            continue;
        }

        out[i] = key; //If other cases are not true, just copy the char
    }
    out[i] = 0x00; //Add null terminator
}