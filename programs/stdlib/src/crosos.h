#ifndef CROSOS_H
#define CROSOS_H
#include <stddef.h>
#include <stdbool.h>
void print(const char* message);
int crosos_getkey();
void* crosos_malloc(size_t size);
void crosos_free(void* ptr);
void crosos_putchar(char c);
void crosos_terminal_readline(char* out, int max, bool output_while_typing);
int crosos_getkeyblock();
void crosos_process_load_start(const char* filename);

#endif