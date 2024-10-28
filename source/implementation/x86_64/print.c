// print.c
#include "print.h"
#include "memory.h"
#include "console.h"
#include <stdint.h>

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
static size_t col = 0;
static size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

// Map basic scan codes to ASCII characters for lowercase letters and numbers
static const char key_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Map for uppercase letters and shifted characters
static const char shifted_key_map[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void print_hex_digit(unsigned char digit) {
    if (digit < 10) {
        print_char('0' + digit);
    } else {
        print_char('A' + (digit - 10));
    }
}

void print_hex(unsigned int number) {
    // Handle special case of 0
    if (number == 0) {
        print_str("0x0");
        return;
    }

    print_str("0x");
    
    // Find first non-zero digit
    int started = 0;
    for (int i = 7; i >= 0; i--) {
        unsigned char digit = (number >> (i * 4)) & 0xF;
        if (digit != 0 || started) {
            print_hex_digit(digit);
            started = 1;
        }
    }
}

void print_dec(unsigned int number) {
    char buffer[11];  // Maximum 10 digits for 32-bit number plus null terminator
    int i = 0;
    
    // Handle special case of 0
    if (number == 0) {
        print_char('0');
        return;
    }
    
    // Convert number to string (in reverse)
    while (number > 0) {
        buffer[i++] = '0' + (number % 10);
        number /= 10;
    }
    
    // Print in correct order
    while (i > 0) {
        print_char(buffer[--i]);
    }
}

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_FULL 0x01
#define LEFT_SHIFT_PRESSED  0x2A
#define RIGHT_SHIFT_PRESSED 0x36
#define LEFT_SHIFT_RELEASED 0xAA
#define RIGHT_SHIFT_RELEASED 0xB6
#define CAPS_LOCK_PRESSED 0x3A

static int caps_lock_enabled = 0;
static int shift_pressed = 0;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };
    
    // Use memory_set to clear the entire row with the empty character
    for (size_t col = 0; col < NUM_COLS; col++) {
        memory_copy(&buffer[col + NUM_COLS * row], &empty, sizeof(struct Char));
    }
}

void print_clear() {
    row = 0;
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
    } else {
        // Use memory_move to scroll the screen up
        memory_move(buffer, buffer + NUM_COLS, (NUM_ROWS - 1) * NUM_COLS * sizeof(struct Char));
        clear_row(NUM_ROWS - 1);
    }
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (character == '\b') {
        if (col == 0 && row > 0) {
            row--;
            col = NUM_COLS - 1;
        } else if (col > 0) {
            col--;
        }

        struct Char empty = (struct Char) {
            character: ' ',
            color: color,
        };
        memory_copy(&buffer[col + NUM_COLS * row], &empty, sizeof(struct Char));
        return;
    }

    if (col >= NUM_COLS) {
        print_newline();
    }

    struct Char ch = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };
    memory_copy(&buffer[col + NUM_COLS * row], &ch, sizeof(struct Char));

    col++;
}

void print_str(char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

void print_int(uint32_t num) {
    char buffer[11];  // Enough for 32-bit integer
    memory_zero(buffer, sizeof(buffer));
    
    if (num == 0) {
        print_str("0");
        return;
    }

    int i = 10;
    while (num > 0 && i > 0) {
        buffer[--i] = '0' + (num % 10);
        num /= 10;
    }

    print_str(&buffer[i]);
}

char get_char() {
    uint8_t scan_code = 0;

    while (1) {
        if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_BUFFER_FULL) {
            scan_code = inb(KEYBOARD_DATA_PORT);

            // Handle Shift key
            if (scan_code == LEFT_SHIFT_PRESSED || scan_code == RIGHT_SHIFT_PRESSED) {
                shift_pressed = 1;
                continue;
            } else if (scan_code == LEFT_SHIFT_RELEASED || scan_code == RIGHT_SHIFT_RELEASED) {
                shift_pressed = 0;
                continue;
            }

            // Handle Caps Lock key
            if (scan_code == CAPS_LOCK_PRESSED) {
                caps_lock_enabled = !caps_lock_enabled;
                continue;
            }

            // Handle key press for normal and shifted/caps lock states
            if (scan_code < 128) {
                if (shift_pressed || caps_lock_enabled) {
                    if (scan_code >= 'a' && scan_code <= 'z' && caps_lock_enabled && !shift_pressed) {
                        return shifted_key_map[scan_code];
                    }
                    return shifted_key_map[scan_code];
                } else {
                    return key_map[scan_code];
                }
            }
        }
    }
}

void read_input(char* buffer, size_t buffer_size) {
    size_t i = 0;
    char c;
    
    memory_zero(buffer, buffer_size);
    
    while (i < buffer_size - 1) {
        c = get_char();

        if (c == '\n' || c == '\r') break;

        if (c == '\b') {
            if (i > 0) {
                i--;
                print_char('\b');
                buffer[i] = '\0';
            }
        } else {
            buffer[i++] = c;
            print_char(c);
        }
    }
    buffer[i] = '\0';
}