
#include <drivers/keyboard.h>

#define SCANCODE_LSHIFT 0x2A
#define SCANCODE_RSHIFT 0x36
#define SCANCODE_LSHIFT_RELEASE 0xAA
#define SCANCODE_RSHIFT_RELEASE 0xB6

static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, 
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, 
    '*',
    0, 
    ' ', 
    0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

static const char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, 
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, 
    '*',
    0, 
    ' ', 
    0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_read_pos = 0;
static int buffer_write_pos = 0;
static bool shift_pressed = false;


bool keyboard_init() {
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    return true;
}


bool keyboard_has_input() {
    return buffer_read_pos != buffer_write_pos;
}


char keyboard_peek() {
    if (!keyboard_has_input()) {
        return 0; 
    }
    return keyboard_buffer[buffer_read_pos];
}


char keyboard_getchar() {
    
    while (!keyboard_has_input()) {
        
        asm volatile("nop");
    }

    char c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}


extern "C" void keyboard_handler() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    
    if (scancode == SCANCODE_LSHIFT || scancode == SCANCODE_RSHIFT) {
        shift_pressed = true;
        return;
    }
    if (scancode == SCANCODE_LSHIFT_RELEASE || scancode == SCANCODE_RSHIFT_RELEASE) {
        shift_pressed = false;
        return;
    }

    
    if (scancode & 0x80) {
        return;
    }

    
    if (scancode < sizeof(scancode_to_ascii)) {
        char ascii;
        if (shift_pressed) {
            ascii = scancode_to_ascii_shift[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
        
        if (ascii != 0) {
            keyboard_buffer[buffer_write_pos] = ascii;
            buffer_write_pos = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
        }
    }
}
