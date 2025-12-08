
#ifndef TIMER_H
#define TIMER_H

#include <kernel/kernel.h>


#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND      0x43


#define PIT_FREQUENCY 1193182


void timer_init(uint32_t frequency);
extern "C" void timer_handler();  
uint32_t timer_get_ticks();
void timer_sleep(uint32_t milliseconds);

#endif 

