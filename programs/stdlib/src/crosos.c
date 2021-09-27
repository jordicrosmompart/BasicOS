#include "crosos.h"

int crosos_getkeyblock()
{
    int val = 0;
    do
    {
        val = crosos_getkey();
    } while(!val);
}

void crosos_terminal_readline(char* out, int max, bool output_while_typing)
{
    int i = 0;
    for (i = 0; i< max-1; i++)
    {
        char key = crosos_getkeyblock();
        //Check if key is 'Enter'
        if(key == 13)
        {
            break;
        }

        if(output_while_typing)
        {
            crosos_putchar(key);
        }

        //Backspace
        if(key == 0x08 && i >= 1)
        {
            out[i-1] = 0x00;
            i -=2; //the iterator will be increased by 
            continue;
        }

        out[i] = key;
    }
    out[i] = 0x00; //Add null terminator
}