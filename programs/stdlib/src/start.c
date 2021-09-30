#include "crosos.h"

extern int main(int argc, char** argv);

//Gets the arguments from the process and calls main
void c_start()
{
    struct process_arguments arguments;
    crosos_process_get_arguments(&arguments); //Gets the arguments from process

    int res = main(arguments.argc, arguments.argv); //Call main
    if(res == 0)
    {

    }
}