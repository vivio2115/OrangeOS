
#include <drivers/timer.h>
#include <kernel/idt.h>
#include <kernel/scheduler.h>


static uint32_t timer_ticks = 0;


bool timer_init(uint32_t frequency) {
    if (frequency == 0 || frequency > PIT_FREQUENCY) {
        return false;
    }
    
    
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    
    
    outb(PIT_COMMAND, 0x36);
    
    
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);
    
    outb(PIT_CHANNEL0_DATA, low);
    outb(PIT_CHANNEL0_DATA, high);
    
    
    timer_ticks = 0;
    
    return true;
}


extern "C" void timer_handler() {
    timer_ticks++;
    
    
    scheduler_schedule();
}


uint32_t timer_get_ticks() {
    return timer_ticks;
}


void timer_sleep(uint32_t milliseconds) {
    
    uint32_t start_ticks = timer_ticks;
    uint32_t ticks_to_wait = milliseconds / 10;
    
    while ((timer_ticks - start_ticks) < ticks_to_wait) {
        asm volatile("hlt");
    }
}

