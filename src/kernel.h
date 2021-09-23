#ifndef kernel_h
#define kernel_h


#define VGA_WIDTH 80
#define VGA_HEIGHT 20
#define CROSOS_MAX_PATH 108

#define ERROR(value) (void*) value
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int) value) < 0

void kernel_main();
void print(const char* str);
void panic(const char* msg);
void kernel_page();
void terminal_writechar(char c, char color);

extern void kernel_registers();

#endif