
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kernel/kernel.h>


#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 256


bool keyboard_init();
char keyboard_getchar();
bool keyboard_has_input();
char keyboard_peek(); 


extern "C" void irq1_handler();


extern "C" void outb(uint16_t port, uint8_t value);
extern "C" uint8_t inb(uint16_t port);

#endif 
