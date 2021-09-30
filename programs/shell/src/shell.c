#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "crosos.h"

//Basic shell, it reads a command typed and opens a process with arguments
int main(int argc, char** argv)
{
    print("CrosOS v1.0.0 initialized\n");
    while(1) 
    {
        print("> ");
        char buff[1024];
        crosos_terminal_readline(buff, sizeof(buff), true);
        print("\n");
        crosos_system_run(buff);
        print("\n");
    }
    return 0;
}