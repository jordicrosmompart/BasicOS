#include "crosos.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char** argv)
{
    printf("My age is %i\n", 24);
    print("Printing from stdlib\n");
    print(itoa(8765));
    void* ptr = malloc(512);

    putchar('Z');
    free(ptr);

    char buff[1024];
    crosos_terminal_readline(buff, sizeof(buff), true);
    print(buff);

    while(1) 
    {
        
    };
    return 0;
}