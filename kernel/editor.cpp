
#include <kernel/editor.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/fat32.h>
#include <kernel/heap.h>
#include <lib/memory.h>


extern "C" void outb(uint16_t port, uint8_t value);

#define EDITOR_MAX_LINES 1000
#define EDITOR_LINE_LENGTH 256
#define EDITOR_BUFFER_SIZE (EDITOR_MAX_LINES * EDITOR_LINE_LENGTH)
#define EDITOR_MAX_UNDO 100
#define EDITOR_COMMAND_BUFFER_SIZE 256


struct editor_state {
    char lines[EDITOR_MAX_LINES][EDITOR_LINE_LENGTH];
    int line_count;
    int cursor_line;
    int cursor_col;
};

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


static struct editor_state* undo_stack[EDITOR_MAX_UNDO];
static int undo_count = 0;
static struct editor_state* redo_stack[EDITOR_MAX_UNDO];
static int redo_count = 0;


static char search_pattern[256] = "";
static char replace_pattern[256] = "";
static int search_match_line = -1;
static int search_match_col = -1;


static char command_buffer[EDITOR_COMMAND_BUFFER_SIZE] = "";
static int command_cursor = 0;


static int visual_start_line = -1;
static int visual_start_col = -1;


static char last_command = 0;
static int command_repeat = 0;



static void editor_save_state() {
    
    if (undo_count >= EDITOR_MAX_UNDO) {
        kfree(undo_stack[0]);
        for (int i = 0; i < EDITOR_MAX_UNDO - 1; i++) {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_count--;
    }
    
    struct editor_state* state = (struct editor_state*)kmalloc(sizeof(struct editor_state));
    if (state == NULL) return;
    
    memcpy(state->lines, editor_lines, sizeof(editor_lines));
    state->line_count = editor_line_count;
    state->cursor_line = editor_cursor_line;
    state->cursor_col = editor_cursor_col;
    
    undo_stack[undo_count++] = state;
    
    
    for (int i = 0; i < redo_count; i++) {
        kfree(redo_stack[i]);
    }
    redo_count = 0;
}

static void editor_undo() {
    if (undo_count == 0) {
        strncpy(editor_message, "Nothing to undo", 79);
        return;
    }
    
    
    if (redo_count < EDITOR_MAX_UNDO) {
        struct editor_state* state = (struct editor_state*)kmalloc(sizeof(struct editor_state));
        if (state != NULL) {
            memcpy(state->lines, editor_lines, sizeof(editor_lines));
            state->line_count = editor_line_count;
            state->cursor_line = editor_cursor_line;
            state->cursor_col = editor_cursor_col;
            redo_stack[redo_count++] = state;
        }
    }
    
    
    struct editor_state* state = undo_stack[--undo_count];
    memcpy(editor_lines, state->lines, sizeof(editor_lines));
    editor_line_count = state->line_count;
    editor_cursor_line = state->cursor_line;
    editor_cursor_col = state->cursor_col;
    kfree(state);
    
    editor_modified = true;
    strncpy(editor_message, "Undo successful", 79);
}

static void editor_redo() {
    if (redo_count == 0) {
        strncpy(editor_message, "Nothing to redo", 79);
        return;
    }
    
    
    if (undo_count < EDITOR_MAX_UNDO) {
        struct editor_state* state = (struct editor_state*)kmalloc(sizeof(struct editor_state));
        if (state != NULL) {
            memcpy(state->lines, editor_lines, sizeof(editor_lines));
            state->line_count = editor_line_count;
            state->cursor_line = editor_cursor_line;
            state->cursor_col = editor_cursor_col;
            undo_stack[undo_count++] = state;
        }
    }
    
    
    struct editor_state* state = redo_stack[--redo_count];
    memcpy(editor_lines, state->lines, sizeof(editor_lines));
    editor_line_count = state->line_count;
    editor_cursor_line = state->cursor_line;
    editor_cursor_col = state->cursor_col;
    kfree(state);
    
    editor_modified = true;
    strncpy(editor_message, "Redo successful", 79);
}


static bool editor_search_forward() {
    if (search_pattern[0] == '\0') {
        strncpy(editor_message, "No search pattern", 79);
        return false;
    }
    
    
    int start_line = editor_cursor_line;
    int start_col = editor_cursor_col + 1;
    
    for (int i = start_line; i < editor_line_count; i++) {
        char* line = editor_lines[i];
        int search_from = (i == start_line) ? start_col : 0;
        
        for (int j = search_from; line[j] != '\0'; j++) {
            bool match = true;
            for (int k = 0; search_pattern[k] != '\0'; k++) {
                if (line[j + k] != search_pattern[k]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                editor_cursor_line = i;
                editor_cursor_col = j;
                search_match_line = i;
                search_match_col = j;
                
                
                if (editor_cursor_line < editor_top_line) {
                    editor_top_line = editor_cursor_line;
                } else if (editor_cursor_line >= editor_top_line + 24) {
                    editor_top_line = editor_cursor_line - 23;
                }
                
                return true;
            }
        }
    }
    
    strncpy(editor_message, "Pattern not found", 79);
    return false;
}

static bool editor_search_backward() {
    if (search_pattern[0] == '\0') {
        strncpy(editor_message, "No search pattern", 79);
        return false;
    }
    
    
    int start_line = editor_cursor_line;
    int start_col = editor_cursor_col - 1;
    
    for (int i = start_line; i >= 0; i--) {
        char* line = editor_lines[i];
        int line_len = strlen(line);
        int search_from = (i == start_line) ? start_col : line_len - 1;
        
        for (int j = search_from; j >= 0; j--) {
            bool match = true;
            for (int k = 0; search_pattern[k] != '\0'; k++) {
                if (line[j + k] != search_pattern[k]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                editor_cursor_line = i;
                editor_cursor_col = j;
                search_match_line = i;
                search_match_col = j;
                
                
                if (editor_cursor_line < editor_top_line) {
                    editor_top_line = editor_cursor_line;
                } else if (editor_cursor_line >= editor_top_line + 24) {
                    editor_top_line = editor_cursor_line - 23;
                }
                
                return true;
            }
        }
    }
    
    strncpy(editor_message, "Pattern not found", 79);
    return false;
}

static void editor_replace_current() {
    if (search_pattern[0] == '\0') {
        strncpy(editor_message, "No search pattern", 79);
        return;
    }
    
    editor_save_state();
    
    char* line = editor_lines[editor_cursor_line];
    int search_len = strlen(search_pattern);
    int replace_len = strlen(replace_pattern);
    int line_len = strlen(line);
    
    
    bool match = true;
    for (int k = 0; search_pattern[k] != '\0'; k++) {
        if (line[editor_cursor_col + k] != search_pattern[k]) {
            match = false;
            break;
        }
    }
    
    if (!match) {
        strncpy(editor_message, "No match at cursor", 79);
        return;
    }
    
    
    if (line_len - search_len + replace_len >= EDITOR_LINE_LENGTH) {
        strncpy(editor_message, "Line too long after replace", 79);
        return;
    }
    
    
    if (replace_len != search_len) {
        int shift = replace_len - search_len;
        if (shift > 0) {
            for (int i = line_len; i >= editor_cursor_col + search_len; i--) {
                line[i + shift] = line[i];
            }
        } else {
            for (int i = editor_cursor_col + search_len; i <= line_len; i++) {
                line[i + shift] = line[i];
            }
        }
    }
    
    
    for (int i = 0; i < replace_len; i++) {
        line[editor_cursor_col + i] = replace_pattern[i];
    }
    
    editor_modified = true;
    strncpy(editor_message, "Replaced 1 occurrence", 79);
}

static void editor_replace_all() {
    if (search_pattern[0] == '\0') {
        strncpy(editor_message, "No search pattern", 79);
        return;
    }
    
    editor_save_state();
    
    int count = 0;
    int search_len = strlen(search_pattern);
    int replace_len = strlen(replace_pattern);
    
    for (int i = 0; i < editor_line_count; i++) {
        char* line = editor_lines[i];
        int j = 0;
        
        while (line[j] != '\0') {
            bool match = true;
            for (int k = 0; search_pattern[k] != '\0'; k++) {
                if (line[j + k] != search_pattern[k]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                int line_len = strlen(line);
                
                
                if (line_len - search_len + replace_len >= EDITOR_LINE_LENGTH) {
                    j++;
                    continue;
                }
                
                
                if (replace_len != search_len) {
                    int shift = replace_len - search_len;
                    if (shift > 0) {
                        for (int m = line_len; m >= j + search_len; m--) {
                            line[m + shift] = line[m];
                        }
                    } else {
                        for (int m = j + search_len; m <= line_len; m++) {
                            line[m + shift] = line[m];
                        }
                    }
                }
                
                
                for (int m = 0; m < replace_len; m++) {
                    line[j + m] = replace_pattern[m];
                }
                
                count++;
                j += replace_len;
            } else {
                j++;
            }
        }
    }
    
    editor_modified = true;
    char msg[80];
    strncpy(msg, "Replaced ", 79);
    char count_str[32];
    itoa(count, count_str, 10);
    int msg_len = strlen(msg);
    if (msg_len < 79) {
        int remaining = 79 - msg_len;
        for (int i = 0; count_str[i] != '\0' && i < remaining - 1; i++) {
            msg[msg_len++] = count_str[i];
        }
        msg[msg_len] = '\0';
    }
    msg_len = strlen(msg);
    if (msg_len < 79) {
        const char* suffix = " occurrences";
        int remaining = 79 - msg_len;
        for (int i = 0; suffix[i] != '\0' && i < remaining - 1; i++) {
            msg[msg_len++] = suffix[i];
        }
        msg[msg_len] = '\0';
    }
    strncpy(editor_message, msg, 79);
}


static uint8_t editor_get_syntax_color(const char* line, int col) {
    
    char c = line[col];
    
    
    int in_string = 0;
    for (int i = 0; i < col; i++) {
        if (line[i] == '"' && (i == 0 || line[i-1] != '\\')) {
            in_string = !in_string;
        }
    }
    if (in_string) return VGA_COLOR_YELLOW;
    
    
    if (col > 0 && line[col-1] == '/' && line[col] == '/') {
        return VGA_COLOR_GREEN;
    }
    if (col > 0) {
        for (int i = col; i >= 1; i--) {
            if (line[i-1] == '/' && line[i] == '/') {
                return VGA_COLOR_GREEN;
            }
        }
    }
    
    
    const char* keywords[] = {"int", "void", "char", "if", "else", "while", "for", "return", "static", "const", "struct", "class", "bool", NULL};
    for (int k = 0; keywords[k] != NULL; k++) {
        int len = strlen(keywords[k]);
        if (col >= len - 1) {
            bool match = true;
            for (int i = 0; i < len; i++) {
                if (line[col - len + 1 + i] != keywords[k][i]) {
                    match = false;
                    break;
                }
            }
            
            if (match && ((size_t)(col + 1) >= strlen(line) || line[col + 1] == ' ' || line[col + 1] == '(' || line[col + 1] == ';')) {
                if (col - len + 1 == 0 || line[col - len] == ' ' || line[col - len] == '\t') {
                    return VGA_COLOR_CYAN;
                }
            }
        }
    }
    
    
    if (c >= '0' && c <= '9') return VGA_COLOR_MAGENTA;
    
    
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>') {
        return VGA_COLOR_LIGHT_RED;
    }
    
    return VGA_COLOR_WHITE;
}

static void editor_init() {
    editor_line_count = 1;
    editor_cursor_line = 0;
    editor_cursor_col = 0;
    editor_top_line = 0;
    editor_mode = EDITOR_MODE_NORMAL;
    editor_modified = false;
    memset(editor_lines, 0, sizeof(editor_lines));
    editor_lines[0][0] = '\0';
    
    
    for (int i = 0; i < undo_count; i++) {
        kfree(undo_stack[i]);
    }
    undo_count = 0;
    for (int i = 0; i < redo_count; i++) {
        kfree(redo_stack[i]);
    }
    redo_count = 0;
    
    search_pattern[0] = '\0';
    replace_pattern[0] = '\0';
    search_match_line = -1;
    search_match_col = -1;
    visual_start_line = -1;
    visual_start_col = -1;
    command_buffer[0] = '\0';
    command_cursor = 0;
    last_command = 0;
    command_repeat = 0;
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
    
    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
    
    
    auto make_entry = [](char c, uint8_t fg, uint8_t bg) -> uint16_t {
        uint8_t color = fg | (bg << 4);
        return (uint16_t)c | ((uint16_t)color << 8);
    };
    
    int vga_pos = 0;
    
    
    const char* mode_str = "";
    if (editor_mode == EDITOR_MODE_INSERT) {
        mode_str = "-- INSERT --";
    } else if (editor_mode == EDITOR_MODE_VISUAL) {
        mode_str = "-- VISUAL --";
    } else if (editor_mode == EDITOR_MODE_COMMAND) {
        mode_str = "-- COMMAND --";
    } else if (editor_mode == EDITOR_MODE_SEARCH) {
        mode_str = "-- SEARCH --";
    } else {
        mode_str = "-- NORMAL --";
    }
    
    char status[80];
    int pos = 0;
    
    
    for (int i = 0; editor_filename[i] != '\0' && pos < 70; i++) {
        status[pos++] = editor_filename[i];
    }
    status[pos++] = ' ';
    
    
    for (int i = 0; mode_str[i] != '\0' && pos < 70; i++) {
        status[pos++] = mode_str[i];
    }
    status[pos++] = ' ';
    
    
    if (editor_modified) {
        status[pos++] = '[';
        status[pos++] = '+';
        status[pos++] = ']';
        status[pos++] = ' ';
    }
    
    
    char num_buf[16];
    itoa(editor_cursor_line + 1, num_buf, 10);
    for (int i = 0; num_buf[i] != '\0' && pos < 78; i++) {
        status[pos++] = num_buf[i];
    }
    status[pos++] = ',';
    itoa(editor_cursor_col + 1, num_buf, 10);
    for (int i = 0; num_buf[i] != '\0' && pos < 78; i++) {
        status[pos++] = num_buf[i];
    }
    status[pos++] = '/';
    itoa(editor_line_count, num_buf, 10);
    for (int i = 0; num_buf[i] != '\0' && pos < 79; i++) {
        status[pos++] = num_buf[i];
    }
    
    
    for (int i = 0; i < 80; i++) {
        char c = (i < pos) ? status[i] : ' ';
        vga_buffer[vga_pos++] = make_entry(c, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    }
    
    
    for (int screen_line = 0; screen_line < 23; screen_line++) {
        int line_idx = editor_top_line + screen_line;
        
        if (line_idx >= editor_line_count) {
            
            vga_buffer[vga_pos++] = make_entry('~', VGA_COLOR_BLUE, VGA_COLOR_BLACK);
            for (int i = 1; i < 80; i++) {
                vga_buffer[vga_pos++] = make_entry(' ', VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            }
            continue;
        }
        
        char* line = editor_lines[line_idx];
        int len = (int)strlen(line);
        
        
        bool in_visual = false;
        int visual_start = 0, visual_end = 0;
        if (editor_mode == EDITOR_MODE_VISUAL && visual_start_line >= 0) {
            if (line_idx >= visual_start_line && line_idx <= editor_cursor_line) {
                in_visual = true;
                visual_start = (line_idx == visual_start_line) ? visual_start_col : 0;
                visual_end = (line_idx == editor_cursor_line) ? editor_cursor_col : len;
            } else if (line_idx >= editor_cursor_line && line_idx <= visual_start_line) {
                in_visual = true;
                visual_start = (line_idx == editor_cursor_line) ? editor_cursor_col : 0;
                visual_end = (line_idx == visual_start_line) ? visual_start_col : len;
            }
        }
        
        
        for (int col = 0; col < 80; col++) {
            char c = (col < len) ? line[col] : ' ';
            
            
            uint8_t fg = VGA_COLOR_WHITE;
            uint8_t bg = VGA_COLOR_BLACK;
            
            if (col < len) {
                
                bool is_match = false;
                if (search_match_line == line_idx && search_match_col >= 0) {
                    int match_len = (int)strlen(search_pattern);
                    if (col >= search_match_col && col < search_match_col + match_len) {
                        is_match = true;
                    }
                }
                
                if (is_match) {
                    fg = VGA_COLOR_BLACK;
                    bg = VGA_COLOR_YELLOW;
                } else if (in_visual && col >= visual_start && col <= visual_end) {
                    fg = VGA_COLOR_WHITE;
                    bg = VGA_COLOR_BLUE;
                } else {
                    
                    fg = editor_get_syntax_color(line, col);
                    
                    if (line_idx == editor_cursor_line && editor_mode == EDITOR_MODE_NORMAL) {
                        bg = VGA_COLOR_DARK_GREY;
                    } else {
                        bg = VGA_COLOR_BLACK;
                    }
                }
            } else {
                if (line_idx == editor_cursor_line && editor_mode == EDITOR_MODE_NORMAL) {
                    bg = VGA_COLOR_DARK_GREY;
                }
            }
            
            vga_buffer[vga_pos++] = make_entry(c, fg, bg);
        }
    }
    
    
    char cmd_line[80];
    int cmd_pos = 0;
    
    if (editor_mode == EDITOR_MODE_COMMAND || editor_mode == EDITOR_MODE_SEARCH) {
        char prefix = (editor_mode == EDITOR_MODE_SEARCH) ? '/' : ':';
        cmd_line[cmd_pos++] = prefix;
        for (int i = 0; command_buffer[i] != '\0' && cmd_pos < 79; i++) {
            cmd_line[cmd_pos++] = command_buffer[i];
        }
    } else if (editor_message[0] != '\0') {
        for (int i = 0; editor_message[i] != '\0' && cmd_pos < 79; i++) {
            cmd_line[cmd_pos++] = editor_message[i];
        }
    }
    
    for (int i = 0; i < 80; i++) {
        char c = (i < cmd_pos) ? cmd_line[i] : ' ';
        vga_buffer[vga_pos++] = make_entry(c, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    
    int cursor_x = editor_cursor_col;
    int cursor_y = (editor_cursor_line - editor_top_line) + 1; 
    
    if (editor_mode == EDITOR_MODE_COMMAND || editor_mode == EDITOR_MODE_SEARCH) {
        cursor_x = command_cursor + 1; 
        cursor_y = 24;
    }
    
    
    if (cursor_x > 79) cursor_x = 79;
    if (cursor_y < 1) cursor_y = 1;
    if (cursor_y > 24) cursor_y = 24;
    
    
    uint16_t cursor_pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(cursor_pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((cursor_pos >> 8) & 0xFF));
}


static void editor_handle_normal_mode(char c) {
    editor_message[0] = '\0';
    
    
    if (c >= '1' && c <= '9' && last_command == 0) {
        command_repeat = command_repeat * 10 + (c - '0');
        return;
    }
    
    int repeat = (command_repeat > 0) ? command_repeat : 1;
    command_repeat = 0;
    
    switch (c) {
        
        case 'i':
            editor_mode = EDITOR_MODE_INSERT;
            editor_save_state();
            break;
        case 'I': 
            editor_mode = EDITOR_MODE_INSERT;
            editor_cursor_col = 0;
            editor_save_state();
            break;
        case 'a':
            editor_mode = EDITOR_MODE_INSERT;
            if (editor_cursor_col < (int)strlen(editor_lines[editor_cursor_line])) {
                editor_cursor_col++;
            }
            editor_save_state();
            break;
        case 'A': 
            editor_mode = EDITOR_MODE_INSERT;
            editor_cursor_col = strlen(editor_lines[editor_cursor_line]);
            editor_save_state();
            break;
            
        
        case 'h':
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_col > 0) editor_cursor_col--;
            }
            break;
        case 'l':
            for (int i = 0; i < repeat; i++) {
                int len = (int)strlen(editor_lines[editor_cursor_line]);
                if (editor_cursor_col < len) editor_cursor_col++;
            }
            break;
        case 'j':
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_line < editor_line_count - 1) {
                    editor_cursor_line++;
                    if (editor_cursor_line >= editor_top_line + 23) {
                        editor_top_line = editor_cursor_line - 22;
                    }
                    int len = strlen(editor_lines[editor_cursor_line]);
                    if (editor_cursor_col > len) editor_cursor_col = len;
                }
            }
            break;
        case 'k':
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_line > 0) {
                    editor_cursor_line--;
                    if (editor_cursor_line < editor_top_line) {
                        editor_top_line = editor_cursor_line;
                    }
                    int len = strlen(editor_lines[editor_cursor_line]);
                    if (editor_cursor_col > len) editor_cursor_col = len;
                }
            }
            break;
        case 'w': 
            for (int i = 0; i < repeat; i++) {
                char* line = editor_lines[editor_cursor_line];
                int len = strlen(line);
                
                while (editor_cursor_col < len && line[editor_cursor_col] != ' ') editor_cursor_col++;
                
                while (editor_cursor_col < len && line[editor_cursor_col] == ' ') editor_cursor_col++;
            }
            break;
        case 'b': 
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_col > 0) {
                    editor_cursor_col--;
                    char* line = editor_lines[editor_cursor_line];
                    
                    while (editor_cursor_col > 0 && line[editor_cursor_col] == ' ') editor_cursor_col--;
                    
                    while (editor_cursor_col > 0 && line[editor_cursor_col - 1] != ' ') editor_cursor_col--;
                }
            }
            break;
        case '0': 
            editor_cursor_col = 0;
            break;
        case '$': 
            editor_cursor_col = strlen(editor_lines[editor_cursor_line]);
            if (editor_cursor_col > 0) editor_cursor_col--;
            break;
        case 'g': 
            editor_cursor_line = 0;
            editor_top_line = 0;
            editor_cursor_col = 0;
            break;
        case 'G': 
            editor_cursor_line = editor_line_count - 1;
            editor_top_line = (editor_line_count > 23) ? editor_line_count - 23 : 0;
            editor_cursor_col = 0;
            break;
            
        
        case 'x':
            editor_save_state();
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_col < (int)strlen(editor_lines[editor_cursor_line])) {
                    int len = (int)strlen(editor_lines[editor_cursor_line]);
                    for (int j = editor_cursor_col; j < len; j++) {
                        editor_lines[editor_cursor_line][j] = editor_lines[editor_cursor_line][j + 1];
                    }
                    editor_modified = true;
                }
            }
            break;
        case 'X': 
            editor_save_state();
            for (int i = 0; i < repeat; i++) {
                if (editor_cursor_col > 0) {
                    editor_cursor_col--;
                    int len = strlen(editor_lines[editor_cursor_line]);
                    for (int j = editor_cursor_col; j < len; j++) {
                        editor_lines[editor_cursor_line][j] = editor_lines[editor_cursor_line][j + 1];
                    }
                    editor_modified = true;
                }
            }
            break;
        case 'd': 
            editor_save_state();
            if (editor_line_count > 1) {
                for (int i = editor_cursor_line; i < editor_line_count - 1; i++) {
                    strcpy(editor_lines[i], editor_lines[i + 1]);
                }
                editor_line_count--;
                if (editor_cursor_line >= editor_line_count) {
                    editor_cursor_line = editor_line_count - 1;
                }
                editor_modified = true;
            } else {
                editor_lines[0][0] = '\0';
                editor_modified = true;
            }
            break;
        case 'o': 
            editor_save_state();
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
            editor_save_state();
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
        case 'r': 
            
            last_command = 'r';
            break;
            
        
        case 'v':
            editor_mode = EDITOR_MODE_VISUAL;
            visual_start_line = editor_cursor_line;
            visual_start_col = editor_cursor_col;
            break;
            
        
        case 'u':
            editor_undo();
            break;
        case 'R': 
            editor_redo();
            break;
            
        
        case '/':
            editor_mode = EDITOR_MODE_SEARCH;
            command_buffer[0] = '\0';
            command_cursor = 0;
            break;
        case 'n': 
            editor_search_forward();
            break;
        case 'N': 
            editor_search_backward();
            break;
            
        
        case ':':
            editor_mode = EDITOR_MODE_COMMAND;
            command_buffer[0] = '\0';
            command_cursor = 0;
            break;
    }
    
    last_command = 0;
}


static void editor_handle_insert_mode(char c) {
    if (c == 27) { 
        editor_mode = EDITOR_MODE_NORMAL;
        if (editor_cursor_col > 0 && editor_cursor_col >= (int)strlen(editor_lines[editor_cursor_line])) {
            editor_cursor_col--;
        }
        return;
    }
    
    if (c == '\b') { 
        if (editor_cursor_col > 0) {
            int len = (int)strlen(editor_lines[editor_cursor_line]);
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
        int len = (int)strlen(editor_lines[editor_cursor_line]);
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

static void editor_handle_visual_mode(char c) {
    if (c == 27) { 
        editor_mode = EDITOR_MODE_NORMAL;
        visual_start_line = -1;
        visual_start_col = -1;
        return;
    }
    
    
    switch (c) {
        case 'h':
            if (editor_cursor_col > 0) editor_cursor_col--;
            break;
        case 'l':
            if (editor_cursor_col < (int)strlen(editor_lines[editor_cursor_line])) {
                editor_cursor_col++;
            }
            break;
        case 'j':
            if (editor_cursor_line < editor_line_count - 1) {
                editor_cursor_line++;
                if (editor_cursor_line >= editor_top_line + 23) {
                    editor_top_line = editor_cursor_line - 22;
                }
            }
            break;
        case 'k':
            if (editor_cursor_line > 0) {
                editor_cursor_line--;
                if (editor_cursor_line < editor_top_line) {
                    editor_top_line = editor_cursor_line;
                }
            }
            break;
        case 'd': 
            editor_save_state();
            
            if (editor_line_count > 1) {
                for (int i = editor_cursor_line; i < editor_line_count - 1; i++) {
                    strcpy(editor_lines[i], editor_lines[i + 1]);
                }
                editor_line_count--;
                if (editor_cursor_line >= editor_line_count) {
                    editor_cursor_line = editor_line_count - 1;
                }
                editor_modified = true;
            }
            editor_mode = EDITOR_MODE_NORMAL;
            visual_start_line = -1;
            visual_start_col = -1;
            break;
    }
}

static void editor_execute_command() {
    if (strcmp(command_buffer, "w") == 0 || strcmp(command_buffer, "write") == 0) {
        
        void* save_buffer = kmalloc(EDITOR_BUFFER_SIZE);
        if (save_buffer == NULL) {
            strncpy(editor_message, "Out of memory!", 79);
            return;
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
        } else {
            strncpy(editor_message, "Save failed!", 79);
        }
        
        kfree(save_buffer);
    } else if (strcmp(command_buffer, "q") == 0 || strcmp(command_buffer, "quit") == 0) {
        if (editor_modified) {
            strncpy(editor_message, "Unsaved changes! Use :q! or :wq", 79);
        } else {
            editor_mode = EDITOR_MODE_NORMAL;
            
            command_buffer[0] = 'Q';
            command_buffer[1] = '\0';
        }
    } else if (strcmp(command_buffer, "q!") == 0) {
        editor_mode = EDITOR_MODE_NORMAL;
        command_buffer[0] = 'Q';
        command_buffer[1] = '\0';
    } else if (strcmp(command_buffer, "wq") == 0 || strcmp(command_buffer, "x") == 0) {
        
        void* save_buffer = kmalloc(EDITOR_BUFFER_SIZE);
        if (save_buffer != NULL) {
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
                command_buffer[0] = 'Q';
                command_buffer[1] = '\0';
            } else {
                strncpy(editor_message, "Save failed!", 79);
            }
            
            kfree(save_buffer);
        }
    } else if (command_buffer[0] == 's' && command_buffer[1] == '/') {
        
        int i = 2;
        int search_idx = 0;
        while (command_buffer[i] != '/' && command_buffer[i] != '\0' && search_idx < 255) {
            search_pattern[search_idx++] = command_buffer[i++];
        }
        search_pattern[search_idx] = '\0';
        
        if (command_buffer[i] == '/') {
            i++;
            int replace_idx = 0;
            while (command_buffer[i] != '/' && command_buffer[i] != '\0' && replace_idx < 255) {
                replace_pattern[replace_idx++] = command_buffer[i++];
            }
            replace_pattern[replace_idx] = '\0';
            
            
            if (command_buffer[i] == '/' && command_buffer[i+1] == 'g') {
                editor_replace_all();
            } else {
                editor_replace_current();
            }
        }
    } else {
        strncpy(editor_message, "Unknown command", 79);
    }
}

static void editor_handle_command_mode(char c) {
    if (c == 27) { 
        editor_mode = EDITOR_MODE_NORMAL;
        command_buffer[0] = '\0';
        command_cursor = 0;
        return;
    }
    
    if (c == '\n') { 
        editor_execute_command();
        if (command_buffer[0] != 'Q') {
            editor_mode = EDITOR_MODE_NORMAL;
        }
        return;
    }
    
    if (c == '\b') { 
        if (command_cursor > 0) {
            command_cursor--;
            command_buffer[command_cursor] = '\0';
        }
        return;
    }
    
    if (c >= 32 && c < 127 && command_cursor < EDITOR_COMMAND_BUFFER_SIZE - 1) {
        command_buffer[command_cursor++] = c;
        command_buffer[command_cursor] = '\0';
    }
}

static void editor_handle_search_mode(char c) {
    if (c == 27) { 
        editor_mode = EDITOR_MODE_NORMAL;
        command_buffer[0] = '\0';
        command_cursor = 0;
        return;
    }
    
    if (c == '\n') { 
        strcpy(search_pattern, command_buffer);
        editor_search_forward();
        editor_mode = EDITOR_MODE_NORMAL;
        return;
    }
    
    if (c == '\b') { 
        if (command_cursor > 0) {
            command_cursor--;
            command_buffer[command_cursor] = '\0';
        }
        return;
    }
    
    if (c >= 32 && c < 127 && command_cursor < EDITOR_COMMAND_BUFFER_SIZE - 1) {
        command_buffer[command_cursor++] = c;
        command_buffer[command_cursor] = '\0';
    }
}


void editor_run() {
    editor_display();
    
    while (true) {
        char c = keyboard_getchar();
        
        if (editor_mode == EDITOR_MODE_NORMAL) {
            if (c == 'q') {
                if (editor_modified) {
                    strncpy(editor_message, "Unsaved changes! Use :wq or :q!", 79);
                    editor_message[79] = '\0';
                    editor_display();
                    continue;
                }
                break;
            }
            editor_handle_normal_mode(c);
        } else if (editor_mode == EDITOR_MODE_INSERT) {
            editor_handle_insert_mode(c);
        } else if (editor_mode == EDITOR_MODE_VISUAL) {
            editor_handle_visual_mode(c);
        } else if (editor_mode == EDITOR_MODE_COMMAND) {
            editor_handle_command_mode(c);
            
            if (command_buffer[0] == 'Q') {
                break;
            }
        } else if (editor_mode == EDITOR_MODE_SEARCH) {
            editor_handle_search_mode(c);
        }
        
        editor_display();
    }
    
    
    if (editor_buffer != NULL) {
        kfree(editor_buffer);
        editor_buffer = NULL;
    }
    
    
    for (int i = 0; i < undo_count; i++) {
        kfree(undo_stack[i]);
    }
    undo_count = 0;
    for (int i = 0; i < redo_count; i++) {
        kfree(redo_stack[i]);
    }
    redo_count = 0;
}

