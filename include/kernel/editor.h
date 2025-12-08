
#ifndef EDITOR_H
#define EDITOR_H

#include <kernel/kernel.h>


enum editor_mode {
    EDITOR_MODE_NORMAL,
    EDITOR_MODE_INSERT
};


void editor_open(const char* filename);
void editor_run();

#endif 

