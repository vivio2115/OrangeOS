
#include <drivers/vga.h>

static volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_MEMORY;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_color_attr = 0x0F; 


extern "C" void outb(uint16_t port, uint8_t value);
extern "C" uint8_t inb(uint16_t port);


void vga_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}


void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}


static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}


static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}


void vga_init() {
    vga_row = 0;
    vga_col = 0;
    vga_color_attr = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    vga_update_cursor(0, 0);
    
    vga_init_custom_palette();
}


void vga_clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color_attr);
        }
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor(0, 0);
}


void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color_attr = vga_color(fg, bg);
}


void vga_scroll() {
    
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color_attr);
    }

    vga_row = VGA_HEIGHT - 1;
}


void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 4) & ~(4 - 1);
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_color_attr);
        }
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_color_attr);
        vga_col++;
    }

    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
    
    
    vga_update_cursor(vga_col, vga_row);
}


void vga_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}


void vga_writeln(const char* str) {
    vga_write(str);
    vga_putchar('\n');
}


void vga_write_at(const char* str, uint8_t x, uint8_t y) {
    if (y >= VGA_HEIGHT) return;
    
    uint8_t saved_row = vga_row;
    uint8_t saved_col = vga_col;
    
    vga_row = y;
    vga_col = x;
    
    for (size_t i = 0; str[i] != '\0' && vga_col < VGA_WIDTH; i++) {
        if (str[i] == '\n') break;
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(str[i], vga_color_attr);
        vga_col++;
    }
    
    vga_row = saved_row;
    vga_col = saved_col;
}


void vga_set_cursor(uint8_t x, uint8_t y) {
    vga_col = x;
    vga_row = y;
    vga_update_cursor(x, y);
}


void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);        
    outb(0x3C9, r & 0x3F);    
    outb(0x3C9, g & 0x3F);    
    outb(0x3C9, b & 0x3F);     
}


void vga_init_custom_palette() {
    vga_set_palette_color(6, 63, 32, 0);  
}
