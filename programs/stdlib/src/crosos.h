#ifndef CROSOS_H
#define CROSOS_H
#include <stddef.h>
#include <stdbool.h>

struct command_argument
{
    char argument[512];
    struct command_argument* next;
};

struct process_arguments
{
    int argc;
    char** argv;
};

void print(const char* message);
int crosos_getkey();
void* crosos_malloc(size_t size);
void crosos_free(void* ptr);
void crosos_putchar(char c);
void crosos_terminal_readline(char* out, int max, bool output_while_typing);
int crosos_getkeyblock();
void crosos_process_load_start(const char* filename);
struct command_argument* crosos_parse_command(const char* command, int max);
int crosos_system(struct command_argument* arguments);
void crosos_process_get_arguments(struct process_arguments* arguments);
int crosos_system_run(const char* command);
void crosos_exit();

#endif