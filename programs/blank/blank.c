#include "crosos.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

//Example program to test the system calls
int main(int argc, char** argv)
{
    char* ptr = malloc(20);
    strcpy(ptr, "Hello world");
    print(ptr);
    free(ptr);

    ptr[0] = 'B';
    while(1) 
    {
        
    };
    return 0;
}