#ifndef SPEAKER_H
#define SPEAKER_H

#include <kernel/kernel.h>

namespace Speaker {
    void init();
    
    void play_sound(uint32_t frequency);
    
    void stop_sound();
    
    void beep(uint32_t frequency, uint32_t duration_ms);
    
    void beep_error();      
    void beep_success();   
    void beep_startup();   
    
    void play_melody();     
}

#endif
