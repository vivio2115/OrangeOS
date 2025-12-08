
#include <drivers/timer.h>
#include <kernel/idt.h>


static uint32_t timer_ticks = 0;


void timer_init(uint32_t frequency) {
    
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    
    
    outb(PIT_COMMAND, 0x36);
    
    
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);
    
    outb(PIT_CHANNEL0_DATA, low);
    outb(PIT_CHANNEL0_DATA, high);
    
    
    timer_ticks = 0;
}


extern "C" void timer_handler() {
    timer_ticks++;
    
    
    outb(0x20, 0x20);
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

