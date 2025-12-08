
#ifndef VGA_H
#define VGA_H

#include <kernel/kernel.h>


#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000


enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
        VGA_COLOR_ORANGE = 6,          // orange
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};


void vga_init();
void vga_clear();
void vga_putchar(char c);
void vga_write(const char* str);
void vga_writeln(const char* str);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_scroll();
void vga_write_at(const char* str, uint8_t x, uint8_t y);
void vga_set_cursor(uint8_t x, uint8_t y);
void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vga_init_custom_palette();

#endif 
