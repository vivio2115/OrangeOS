
#include <kernel/editor.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/fat32.h>
#include <kernel/heap.h>
#include <lib/memory.h>

#define EDITOR_MAX_LINES 1000
#define EDITOR_LINE_LENGTH 256
#define EDITOR_BUFFER_SIZE (EDITOR_MAX_LINES * EDITOR_LINE_LENGTH)

static char* editor_buffer = NULL;
static char editor_lines[EDITOR_MAX_LINES][EDITOR_LINE_LENGTH];
static int editor_line_count = 0;
static int editor_cursor_line = 0;
static int editor_cursor_col = 0;
static int editor_top_line = 0;
static enum editor_mode editor_mode = EDITOR_MODE_NORMAL;
static char editor_filename[256];
static bool editor_modified = false;
static char editor_message[80] = "";


static void editor_init() {
    editor_line_count = 1;
    editor_cursor_line = 0;
    editor_cursor_col = 0;
    editor_top_line = 0;
    editor_mode = EDITOR_MODE_NORMAL;
    editor_modified = false;
    memset(editor_lines, 0, sizeof(editor_lines));
    editor_lines[0][0] = '\0';
}


void editor_open(const char* filename) {
    editor_init();
    strncpy(editor_filename, filename, 255);
    editor_filename[255] = '\0';
    
    
    void* file_buffer = kmalloc(65536);
    if (file_buffer == NULL) {
        vga_writeln("Editor: Out of memory");
        return;
    }
    
    if (fat32_read_file(filename, file_buffer, 65536)) {
        
        char* content = (char*)file_buffer;
        int line = 0;
        int col = 0;
        
        for (size_t i = 0; content[i] != '\0' && line < EDITOR_MAX_LINES - 1; i++) {
            if (content[i] == '\n' || content[i] == '\r') {
                editor_lines[line][col] = '\0';
                line++;
                col = 0;
                if (content[i] == '\r' && content[i + 1] == '\n') {
                    i++; 
                }
            } else if (col < EDITOR_LINE_LENGTH - 1) {
                editor_lines[line][col++] = content[i];
            }
        }
        
        editor_lines[line][col] = '\0';
        editor_line_count = line + 1;
        
        vga_write("Editor: Loaded ");
        char line_str[32];
        itoa(editor_line_count, line_str, 10);
        vga_write(line_str);
        vga_writeln(" lines");
    } else {
        vga_write("Editor: File '");
        vga_write(filename);
        vga_writeln("' not found, starting with empty file");
    }
    
    kfree(file_buffer);
}


static void editor_display() {
    vga_clear();
    
    
    vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    vga_write("-- ");
    vga_write(editor_filename);
    vga_write(" -- ");
    if (editor_mode == EDITOR_MODE_INSERT) {
        vga_write("INSERT");
    } else {
        vga_write("NORMAL");
    }
    if (editor_modified) {
        vga_write(" [MODIFIED]");
    }
    vga_write(" -- Line: ");
    char line_str[32];
    itoa(editor_cursor_line + 1, line_str, 10);
    vga_write(line_str);
    vga_write("/");
    itoa(editor_line_count, line_str, 10);
    vga_write(line_str);
    
    
    if (editor_message[0] != '\0') {
        vga_write(" | ");
        vga_write(editor_message);
    }
    
    
    int status_len = strlen(editor_filename) + 20 + (editor_modified ? 12 : 0) + strlen(line_str) * 2 + (editor_message[0] != '\0' ? strlen(editor_message) + 3 : 0);
    for (int i = status_len; i < 80; i++) {
        vga_putchar(' ');
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_writeln("");
    
    
    int lines_to_show = 24;
    if (editor_line_count < lines_to_show) {
        lines_to_show = editor_line_count;
    }
    
    for (int i = 0; i < lines_to_show; i++) {
        int line_idx = editor_top_line + i;
        if (line_idx >= editor_line_count) {
            break;
        }
        
        
        if (line_idx == editor_cursor_line) {
            vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
        } else {
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        
        vga_write(editor_lines[line_idx]);
        
        
        int len = strlen(editor_lines[line_idx]);
        for (int j = len; j < 80; j++) {
            vga_putchar(' ');
        }
        vga_writeln("");
    }
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    
    if (editor_message[0] != '\0') {
        
        for (int i = lines_to_show; i < 24; i++) {
            vga_writeln("");
        }
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_write(editor_message);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        int msg_len = strlen(editor_message);
        for (int i = msg_len; i < 80; i++) {
            vga_putchar(' ');
        }
    }
    
    
    int cursor_y = editor_cursor_line - editor_top_line + 1; 
    int cursor_x = editor_cursor_col;
    if (cursor_x > strlen(editor_lines[editor_cursor_line])) {
        cursor_x = strlen(editor_lines[editor_cursor_line]);
    }
    
    
    
}


static void editor_handle_normal_mode(char c) {
    
    editor_message[0] = '\0';
    
    switch (c) {
        case 'i':
            editor_mode = EDITOR_MODE_INSERT;
            break;
        case 'a':
            editor_mode = EDITOR_MODE_INSERT;
            editor_cursor_col = strlen(editor_lines[editor_cursor_line]);
            break;
        case 'h':
            if (editor_cursor_col > 0) {
                editor_cursor_col--;
            }
            break;
        case 'l':
            if (editor_cursor_col < strlen(editor_lines[editor_cursor_line])) {
                editor_cursor_col++;
            }
            break;
        case 'k':
            if (editor_cursor_line > 0) {
                editor_cursor_line--;
                if (editor_cursor_line < editor_top_line) {
                    editor_top_line = editor_cursor_line;
                }
                if (editor_cursor_col > strlen(editor_lines[editor_cursor_line])) {
                    editor_cursor_col = strlen(editor_lines[editor_cursor_line]);
                }
            }
            break;
        case 'j':
            if (editor_cursor_line < editor_line_count - 1) {
                editor_cursor_line++;
                if (editor_cursor_line >= editor_top_line + 24) {
                    editor_top_line = editor_cursor_line - 23;
                }
                if (editor_cursor_col > strlen(editor_lines[editor_cursor_line])) {
                    editor_cursor_col = strlen(editor_lines[editor_cursor_line]);
                }
            }
            break;
        case 'x':
            
            if (editor_cursor_col < strlen(editor_lines[editor_cursor_line])) {
                int len = strlen(editor_lines[editor_cursor_line]);
                for (int i = editor_cursor_col; i < len; i++) {
                    editor_lines[editor_cursor_line][i] = editor_lines[editor_cursor_line][i + 1];
                }
                editor_modified = true;
            }
            break;
        case 'o':
            
            if (editor_line_count < EDITOR_MAX_LINES - 1) {
                for (int i = editor_line_count; i > editor_cursor_line + 1; i--) {
                    strcpy(editor_lines[i], editor_lines[i - 1]);
                }
                editor_lines[editor_cursor_line + 1][0] = '\0';
                editor_cursor_line++;
                editor_cursor_col = 0;
                editor_line_count++;
                editor_mode = EDITOR_MODE_INSERT;
                editor_modified = true;
            }
            break;
        case 'O':
            
            if (editor_line_count < EDITOR_MAX_LINES - 1) {
                for (int i = editor_line_count; i > editor_cursor_line; i--) {
                    strcpy(editor_lines[i], editor_lines[i - 1]);
                }
                editor_lines[editor_cursor_line][0] = '\0';
                editor_cursor_col = 0;
                editor_line_count++;
                editor_mode = EDITOR_MODE_INSERT;
                editor_modified = true;
            }
            break;
        case 'w':
            
            {
                
                strncpy(editor_message, "Saving...", 79);
                editor_message[79] = '\0';
                editor_display(); 
                
                void* save_buffer = kmalloc(EDITOR_BUFFER_SIZE);
                if (save_buffer == NULL) {
                    strncpy(editor_message, "Out of memory!", 79);
                    editor_message[79] = '\0';
                    break;
                }
                
                char* buf = (char*)save_buffer;
                int pos = 0;
                
                for (int i = 0; i < editor_line_count; i++) {
                    int len = strlen(editor_lines[i]);
                    if (pos + len + 1 < EDITOR_BUFFER_SIZE) {
                        memcpy(buf + pos, editor_lines[i], len);
                        pos += len;
                        if (i < editor_line_count - 1) {
                            buf[pos++] = '\n';
                        }
                    }
                }
                
                
                if (fat32_write_file(editor_filename, buf, pos)) {
                    editor_modified = false;
                    strncpy(editor_message, "File saved!", 79);
                    editor_message[79] = '\0';
                } else {
                    
                    strncpy(editor_message, "Save failed! (Need ATA disk, not floppy)", 79);
                    editor_message[79] = '\0';
                }
                
                kfree(save_buffer);
            }
            break;
        case 'q':
            
            return;
    }
}


static void editor_handle_insert_mode(char c) {
    if (c == 27) { 
        editor_mode = EDITOR_MODE_NORMAL;
        return;
    }
    
    if (c == '\b') { 
        if (editor_cursor_col > 0) {
            int len = strlen(editor_lines[editor_cursor_line]);
            for (int i = editor_cursor_col - 1; i < len; i++) {
                editor_lines[editor_cursor_line][i] = editor_lines[editor_cursor_line][i + 1];
            }
            editor_cursor_col--;
            editor_modified = true;
        } else if (editor_cursor_line > 0) {
            
            int prev_len = strlen(editor_lines[editor_cursor_line - 1]);
            int curr_len = strlen(editor_lines[editor_cursor_line]);
            if (prev_len + curr_len < EDITOR_LINE_LENGTH) {
                strcpy(editor_lines[editor_cursor_line - 1] + prev_len, editor_lines[editor_cursor_line]);
                for (int i = editor_cursor_line; i < editor_line_count - 1; i++) {
                    strcpy(editor_lines[i], editor_lines[i + 1]);
                }
                editor_line_count--;
                editor_cursor_line--;
                editor_cursor_col = prev_len;
                editor_modified = true;
            }
        }
        return;
    }
    
    if (c == '\n') { 
        
        if (editor_line_count < EDITOR_MAX_LINES - 1) {
            int len = strlen(editor_lines[editor_cursor_line]);
            for (int i = editor_line_count; i > editor_cursor_line + 1; i--) {
                strcpy(editor_lines[i], editor_lines[i - 1]);
            }
            strcpy(editor_lines[editor_cursor_line + 1], editor_lines[editor_cursor_line] + editor_cursor_col);
            editor_lines[editor_cursor_line][editor_cursor_col] = '\0';
            editor_cursor_line++;
            editor_cursor_col = 0;
            editor_line_count++;
            editor_modified = true;
        }
        return;
    }
    
    
    if (c >= 32 && c < 127) { 
        int len = strlen(editor_lines[editor_cursor_line]);
        if (len < EDITOR_LINE_LENGTH - 1) {
            for (int i = len; i >= editor_cursor_col; i--) {
                editor_lines[editor_cursor_line][i + 1] = editor_lines[editor_cursor_line][i];
            }
            editor_lines[editor_cursor_line][editor_cursor_col] = c;
            editor_cursor_col++;
            editor_modified = true;
        }
    }
}


void editor_run() {
    editor_display();
    
    while (true) {
        char c = keyboard_getchar();
        
        if (editor_mode == EDITOR_MODE_NORMAL) {
            if (c == 'q') {
                
                if (editor_modified) {
                    strncpy(editor_message, "File modified! Use 'w' to save or 'Q' to quit", 79);
                    editor_message[79] = '\0';
                    editor_display();
                    continue;
                }
                break; 
            }
            if (c == 'Q') {
                
                break;
            }
            editor_handle_normal_mode(c);
        } else {
            editor_handle_insert_mode(c);
        }
        
        editor_display();
    }
    
    
    if (editor_buffer != NULL) {
        kfree(editor_buffer);
        editor_buffer = NULL;
    }
}

