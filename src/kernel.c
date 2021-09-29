#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "fs/file.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "string/string.h"
#include "disk/streamer.h"
#include "gdt/gdt.h"
#include "config.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"

uint16_t* video_mem = 0;// memory address to write ASCII to the video memory
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;
uint16_t terminal_make_char(char c, char color) 
{
     return (color << 8) | c; //little endian. The video memory needs to be loaded with ASCII + color code. Little endiannes makes us to do it reversedly
}

void terminal_putchar(int x, int y, char c, char color) 
{
    video_mem[y*VGA_WIDTH + x] = terminal_make_char(c, color);
}

void terminal_backspace()
{
    if(terminal_row == 0 && terminal_col == 0) //Checks if we are at the beginning of the terminal
    {
        return;
    }

    if(terminal_col == 0) //Checks if we are at the beginning of a row
    {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    }

    terminal_col -= 1;

    terminal_writechar(' ', 15); //Writes a whitespace where we want to erase

    if(!terminal_col) //Places the cursor at the previous line if we are at the beginning of the following line
    {
        terminal_row -= 1;
        terminal_col = VGA_WIDTH;
    } 
    terminal_col--; //Substracts a column, to get the position just overwriten with a whitespace
}
void terminal_writechar(char c, char color) 
{
    if(c == '\n') 
    {
        terminal_col=0;
        terminal_row++;
        return;
    }

    if(c == 0x08)
    {
        terminal_backspace();
        return;
    }
    terminal_putchar(terminal_col, terminal_row, c, color);
    terminal_col++;
    if(terminal_col>=VGA_WIDTH) 
    {
        terminal_row++;
        terminal_col = 0;
    }
}

void terminal_initialize () 
{
    video_mem = (uint16_t*) 0xB8000;
    for(int y=0; y<VGA_HEIGHT; y++) {
        for(int x=0; x<VGA_WIDTH; x++) {
            terminal_putchar(x, y, ' ', 0);
        }
    }
}

void print(const char* string) 
{
    size_t len = strlen(string);
    for(int i=0; i<len; i++) {
        terminal_writechar(string[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void panic(const char* msg)
{
    print(msg);
    while(1) {}
}

void kernel_page()
{
    kernel_registers(); //Sets the kernel segment registers to the offset of the GDT for kernel_data
    paging_switch(kernel_chunk); //Loads the page directory of kernel
}

struct tss tss;
struct gdt gdt_real[CROSOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[CROSOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00}, //Null segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A}, //Kernel code segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92}, //Kernel data segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8}, // User code segemnt
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF2}, //User data segment
    {.base = (uint32_t) &tss, .limit = sizeof(tss), .type = 0xE9} //TSS Segment
};

void kernel_main() 
{
    terminal_initialize();
    print("CrosOS initializing\n");

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, CROSOS_TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));

    //Initialize the heap
    kheap_init();

    //Initialize filesystems
    fs_init();

    //Search and initialize hard drive
    disk_search_and_init();

    //Initialize IDT
    idt_init();

    //Init the TSS
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;

    //Load the TSS
    tss_load(0x28); //GDT offset of the TSS segment

    //Setup paging
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL); //Creates a new page directory + tables with the flags specified.
    paging_switch(kernel_chunk); // Loads the new address that contains the address of the new paging directory

    //Enable paging
    enable_paging();

    //Register kernel commands from user land
    isr80h_register_commands();

    //Initialize all system keyboard
    keyboard_init();

    struct process* process = 0;
    int32_t res = process_load_switch("0:/shell.elf", &process);
    if(res != CROSOS_ALL_OK)
    {
        panic("Failed to load blank.elf\n");
    }
    
    task_run_first_ever_task();

    while(1) {}

}