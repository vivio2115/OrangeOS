#include <drivers/speaker.h>
#include <drivers/timer.h>


#define PIT_CHANNEL2_DATA   0x42
#define PIT_COMMAND_PORT    0x43
#define PC_SPEAKER_PORT     0x61


static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

namespace Speaker {

void init() {
    
    stop_sound();
}

void play_sound(uint32_t frequency) {
    if (frequency == 0) {
        stop_sound();
        return;
    }
    
    
    
    uint32_t divisor = 1193180 / frequency;
    
    
    outb(PIT_COMMAND_PORT, 0xB6);
    
    
    outb(PIT_CHANNEL2_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL2_DATA, (uint8_t)((divisor >> 8) & 0xFF));
    
    
    uint8_t tmp = inb(PC_SPEAKER_PORT);
    if (tmp != (tmp | 3)) {
        outb(PC_SPEAKER_PORT, tmp | 3);
    }
}

void stop_sound() {
    
    uint8_t tmp = inb(PC_SPEAKER_PORT) & 0xFC;
    outb(PC_SPEAKER_PORT, tmp);
}

void beep(uint32_t frequency, uint32_t duration_ms) {
    play_sound(frequency);
    timer_sleep(duration_ms);
    stop_sound();
}


void beep_error() {
    
    beep(200, 300);
    timer_sleep(100);
    beep(150, 300);
}

void beep_success() {
    
    beep(523, 100);  
    timer_sleep(50);
    beep(659, 100);  
    timer_sleep(50);
    beep(784, 150);  
}

void beep_startup() {
    
    beep(440, 100);   
    timer_sleep(50);
    beep(554, 100);   
    timer_sleep(50);
    beep(659, 100);   
    timer_sleep(50);
    beep(880, 200);   
}

void play_melody() {
    
    
    beep(659, 150);  
    timer_sleep(30);
    beep(659, 150);  
    timer_sleep(170);
    beep(659, 150);  
    timer_sleep(170);
    beep(523, 150);  
    timer_sleep(30);
    beep(659, 150);  
    timer_sleep(300);
    beep(784, 150);  
    timer_sleep(400);
    beep(392, 150);  
    timer_sleep(400);
    
    
    beep(523, 150);  
    timer_sleep(300);
    beep(392, 150);  
    timer_sleep(300);
    beep(330, 150);  
    timer_sleep(300);
    
    
    beep(440, 150);  
    timer_sleep(300);
    beep(494, 150);  
    timer_sleep(200);
    beep(466, 150);  
    timer_sleep(100);
    beep(440, 150);  
    timer_sleep(300);
    
    
    beep(392, 120);  
    timer_sleep(30);
    beep(659, 120);  
    timer_sleep(30);
    beep(784, 120);  
    timer_sleep(30);
    beep(880, 150);  
    timer_sleep(200);
    beep(698, 150);  
    timer_sleep(100);
    beep(784, 150);  
    timer_sleep(300);
    
    
    beep(659, 150);  
    timer_sleep(200);
    beep(523, 150);  
    timer_sleep(100);
    beep(587, 150);  
    timer_sleep(100);
    beep(494, 150);  
    timer_sleep(300);
}

} 
