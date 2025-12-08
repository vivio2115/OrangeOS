
#ifndef PANIC_H
#define PANIC_H

#include <kernel/kernel.h>


void panic(const char* message);
void panic_with_code(const char* message, uint32_t error_code);

#endif 

