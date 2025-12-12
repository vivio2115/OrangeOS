
#ifndef IDT_H
#define IDT_H

#include <kernel/kernel.h>


struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));


struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));


extern "C" void outb(uint16_t port, uint8_t value);
extern "C" uint8_t inb(uint16_t port);
extern "C" void outw(uint16_t port, uint16_t value);
extern "C" uint16_t inw(uint16_t port);


bool idt_init();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);


extern "C" void idt_flush(uint32_t);


extern "C" {
    void isr0(); void isr1(); void isr2(); void isr3(); void isr4(); void isr5(); void isr6(); void isr7();
    void isr8(); void isr9(); void isr10(); void isr11(); void isr12(); void isr13(); void isr14(); void isr15();
    void isr16(); void isr17(); void isr18(); void isr19(); void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27(); void isr28(); void isr29(); void isr30(); void isr31();
}
extern "C" void irq0(); 
extern "C" void irq1(); 


#endif 
