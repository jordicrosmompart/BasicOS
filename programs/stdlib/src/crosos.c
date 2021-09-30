#include "crosos.h"
#include "string.h"

//Returns a command argument that linked with the following parameters of the command to handle calls to programs with parameters
struct command_argument* crosos_parse_command(const char* command, int max)
{
    struct command_argument* root_command = 0;
    char scommand[1024];
    if(max > (int) sizeof(scommand)) //Command cannot be higher than 1024
    {
        return 0;
    }

    strncpy(scommand, command, sizeof(scommand));
    char* token = strtok(scommand, " "); //Get the first chunk (separated by spaces)
    if(!token)
    {
        goto out;
    }

    root_command = crosos_malloc(sizeof(struct command_argument)); //Allocates the root command
    if(!root_command)
    {
        goto out;
    }

    strncpy(root_command->argument, token, sizeof(root_command->argument)); //Copies the argument of the command
    root_command->next = 0;

    struct command_argument* current = root_command; //Temporary 'current' points to root command
    token = strtok(NULL, " "); //Get next chunk
    while(token != 0)
    {
        struct command_argument* new_command = crosos_malloc(sizeof(new_command->argument)); //Allocate new parameter
        if(!new_command)
        {
            break;
        }
        strncpy(new_command->argument, token, sizeof(new_command->argument)); //Copy the argument to the variable
        new_command->next = 0x00;
        current->next = new_command; //Link previous parsed parameter with the current one
        current = new_command; //Assign the previous to the current to repeat iteration
        token = strtok(NULL, " "); //Get next parameter
    }

out:
    return root_command;
}

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

//Runs a command from shell which contains process name and arguments
int crosos_system_run(const char* command)
{
    char buff[1024];
    strncpy(buff, command, sizeof(buff));
    struct command_argument* root_command_argument = crosos_parse_command(buff, sizeof(buff)); //Parses the string into process + arguments
    if(!root_command_argument)
    {
        return -1;
    }

    return crosos_system(root_command_argument); //Calls the system call
}