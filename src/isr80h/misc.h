#ifndef ISR80H_MISC_H
#define ISR80H_MISC_H

struct interrupt_frame;
void* isr80h_command0_sum(struct interrupt_frame* frame); //The interrupt functions may need registers from te user land
#endif