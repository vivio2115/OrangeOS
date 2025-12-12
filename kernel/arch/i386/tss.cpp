#include <kernel/tss.h>
#include <kernel/gdt.h>
#include <drivers/vga.h>

struct tss_entry tss;

extern "C" void tss_flush(uint32_t tss_selector);

void tss_init(uint32_t kernel_stack) {
    uint8_t* tss_ptr = (uint8_t*)&tss;
    for (uint32_t i = 0; i < sizeof(struct tss_entry); i++) {
        tss_ptr[i] = 0;
    }
    
    tss.ss0 = 0x10;  
    tss.esp0 = kernel_stack;
    
    tss.cs = 0x1B;   
    tss.ss = 0x23;   
    tss.ds = 0x23;
    tss.es = 0x23;
    tss.fs = 0x23;
    tss.gs = 0x23;
    
    tss.iomap_base = sizeof(struct tss_entry);
    
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(struct tss_entry) - 1;
    
    gdt_set_gate(5, base, limit, 0xE9, 0x00);
    
    tss_flush(0x28);
    
    vga_writeln("[TSS] Task State Segment initialized");
}

void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}

