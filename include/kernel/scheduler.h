#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/process.h>

void scheduler_init();
void scheduler_schedule(); 
void scheduler_start();
struct process* scheduler_get_next();
void scheduler_add_process(struct process* proc);

#endif