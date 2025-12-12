
#include <kernel/panic.h>
#include <lib/memory.h>
#include <drivers/vga.h>


void panic(const char* message) {
    
    asm volatile("cli");
    
    
    // vga_clear(); 
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    
    vga_writeln("");
    vga_writeln("========================================");
    vga_writeln("   KERNEL PANIC - SYSTEM HALTED");
    vga_writeln("========================================");
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_RED);
    vga_write("Error: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_writeln(message);
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_RED);
    vga_write("System: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_write(ORANGEOS_NAME);
    vga_write(" v");
    vga_writeln(ORANGEOS_VERSION);
    vga_writeln("");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_RED);
    vga_writeln("The system has been halted to prevent");
    vga_writeln("further damage. Please restart.");
    vga_writeln("");
    vga_writeln("========================================");
    
    
    while (1) {
        asm volatile("hlt");
    }
}


void panic_with_code(const char* message, uint32_t error_code) {
    
    asm volatile("cli");
    
    
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    
    vga_writeln("");
    vga_writeln("========================================");
    vga_writeln("   KERNEL PANIC - SYSTEM HALTED");
    vga_writeln("========================================");
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_RED);
    vga_write("Error: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_writeln(message);
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_RED);
    vga_write("Error Code: 0x");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    char buffer[32];
    itoa((int)error_code, buffer, 10);
    vga_writeln(buffer);
    vga_writeln("");
    
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_RED);
    vga_write("System: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_write(ORANGEOS_NAME);
    vga_write(" v");
    vga_writeln(ORANGEOS_VERSION);
    vga_writeln("");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_RED);
    vga_writeln("The system has been halted to prevent");
    vga_writeln("further damage. Please restart.");
    vga_writeln("");
    vga_writeln("========================================");
    
    
    while (1) {
        asm volatile("hlt");
    }
}

