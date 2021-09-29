#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "crosos.h"

int main(int argc, char** argv)
{
    print("CrosOS v1.0.0 initialized\n");
    while(1) 
    {
        print("> ");
        char buff[1024];
        crosos_terminal_readline(buff, sizeof(buff), true);
        crosos_process_load_start(buff);

        print("\n");
    }
    return 0;
}