#include "stdio.h"
#include "crosos.h"
#include "stdlib.h"
#include <stdarg.h>

//Put char by calling system interrupt
int putchar(int c)
{
    crosos_putchar((char) c);
    return 0;
}

//Formatted print with any amount of parameters
int printf(const char* fmt, ...)
{
    va_list ap; //Other parameters
    const char* p; //Pointer to current char to print
    char* sval; //String parameters 
    int ival; //Int parameters

    va_start(ap, fmt); //Initialize ap to retrieve parameters after the 'fmt' variable
    for(p = fmt; *p; p++) //Set 
    {
        if(*p != '%')
        {
            putchar(*p); //Put 
            continue;
        }

        switch(*++p) //If the char is a %, switch case with the following char
        {
            case 'i': //If it is an 'i'
                ival = va_arg(ap, int); //retrieves the following argument
                print(itoa(ival)); 
                break;
            case 's': //If it is an 's'
                sval = va_arg(ap, char*); //Retrieves a string argument
                print(sval); 
                break;
            default: //Print the following char
                putchar(*p);
                break;
        }
    }
    va_end(ap); //Closes the parameter reading
    return 0;
}