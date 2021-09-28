#include "stdlib.h"
#include "crosos.h"

//Parses an integer to a string
char* itoa(int i)
{
    static char text[12]; //An integer cannot have more than 12 characters
    int loc = 11;
    text[11] = 0; //initialize the output
    char neg = 1;
    if(i >= 0) //Check if integer is positive
    {
        neg = 0; //Not negative
        i = -i; //Invert the value
    }

    while(i)
    {
        text[--loc] = '0' - (i % 10); //ASCII character for 0 - the current last digit (thats why the value of 'i' was inversed)
        i /= 10; //Get the following digit (discard the current one)
    }

    if(loc == 11) //If the number is 0, the upper while would not decrease 'loc'
    {
        text[--loc] = '0'; 
    }

    if(neg)
    {
        text[--loc] = '-'; //If negative, put a '-' at the beggining of the text
    }

    return &text[loc]; //Return pointer to the first position of the texted number
}

//Public function call to system interrupt malloc
void* malloc(size_t size)
{  
    return crosos_malloc(size);
}

//Public function call to system interrupt free
void free(void* ptr)
{
    return crosos_free(ptr);
}