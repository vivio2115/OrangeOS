
#ifndef SHELL_H
#define SHELL_H

#include <kernel/kernel.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16


void shell_init();
void shell_run();
void shell_prompt();
void shell_execute(const char* command);

#endif 
