
#include <kernel/idt.h>
#include <kernel/panic.h>
#include <kernel/paging.h>
#include <drivers/vga.h>


struct idt_entry idt_entries[256];
struct idt_ptr idt_pointer;


extern "C" void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

extern "C" uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

extern "C" void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

extern "C" uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;

    idt_entries[num].selector = selector;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags;
}


bool idt_init() {
    idt_pointer.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_pointer.base = (uint32_t)&idt_entries;

    
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    
    
    
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);  
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);  
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); 
    
    
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    
    
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);

    
    
    
    
    
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    
    outb(0x21, 0x20);  
    outb(0xA1, 0x28);  

    
    outb(0x21, 0x04);  
    outb(0xA1, 0x02);  

    
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    
    outb(0x21, 0xFC);  
    outb(0xA1, 0xFF);  

    
    idt_flush((uint32_t)&idt_pointer);
    
    return true;
}


static const char* exception_messages[] = {
    "Divide by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU error",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception"
};


extern "C" void page_fault_handler(uint32_t error_code) {
    
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    
    bool present = (error_code & 0x1) != 0;      
    bool write = (error_code & 0x2) != 0;         
    bool user = (error_code & 0x4) != 0;         
    bool reserved = (error_code & 0x8) != 0;      
    bool instruction = (error_code & 0x10) != 0;  
    
    
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_writeln("=== PAGE FAULT ===");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_write("Faulting address: 0x");
    char hex_chars[] = "0123456789ABCDEF";
    bool started = false;
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (faulting_address >> (i * 4)) & 0xF;
        if (nibble != 0 || started || i == 0) {
            vga_putchar(hex_chars[nibble]);
            started = true;
        }
    }
    vga_writeln("");
    
    vga_write("Error code: 0x");
    started = false;
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (error_code >> (i * 4)) & 0xF;
        if (nibble != 0 || started || i == 0) {
            vga_putchar(hex_chars[nibble]);
            started = true;
        }
    }
    vga_writeln("");
    
    vga_write("Details: ");
    if (!present) {
        vga_write("Page not present");
    } else {
        vga_write("Protection violation");
    }
    if (write) vga_write(", Write");
    if (user) vga_write(", User mode");
    if (reserved) vga_write(", Reserved bit");
    if (instruction) vga_write(", Instruction fetch");
    vga_writeln("");
    
    
    
    panic("Page fault - system halted");
}

















extern "C" void isr_exception_handler(void* regs_ptr) {
    
    
    uint32_t* stack = (uint32_t*)regs_ptr;
    
    
    uint32_t int_no = stack[9];
    
    
    if (int_no == 14) {
        
        uint32_t error_code = stack[10];
        page_fault_handler(error_code);
        return;
    }
    
    const char* message = "Unknown exception";
    if (int_no < 20) {
        message = exception_messages[int_no];
    }
    
    panic(message);
}
