#include "print.h"
#include "console.h"
#include "string.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
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

// Map basic scan codes to ASCII characters for lowercase letters and numbers
static const char key_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // Backspace
    '\t', // Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // Enter
    0,    // Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    // Left Shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, // Right Shift
    '*',
    0,    // Alt
    ' ',  // Space
    // Other keys omitted for brevity
};

// Map for uppercase letters and shifted characters
static const char shifted_key_map[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', // Backspace
    '\t', // Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', // Enter
    0,    // Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    // Left Shift
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, // Right Shift
    '*',
    0,    // Alt
    ' ',  // Space
    // Other keys omitted for brevity
};

// Track whether Shift is currently pressed
static int shift_pressed = 0;

// Wait until a key is pressed and return the corresponding character
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
                    // Shift or Caps Lock is active
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



const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    row = 0;
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_int(uint32_t num)
{
    char buffer[11]; // Enough for 32-bit integer
    int i = 10;
    buffer[i] = '\0';

    // Handle zero case
    if (num == 0) {
        print_str("0");
        return;
    }

    // Convert number to string (in reverse order)
    while (num > 0 && i > 0) {
        buffer[--i] = '0' + (num % 10);
        num /= 10;
    }

    // Print the resulting string
    print_str(&buffer[i]);
}


void print_newline() {
    col = 0;  // Reset column to the beginning

    if (row < NUM_ROWS - 1) {
        // Move to the next row if not at the last row
        row++;
    } else {
        // Scroll screen by shifting all rows up by one
        for (size_t r = 1; r < NUM_ROWS; r++) {
            for (size_t c = 0; c < NUM_COLS; c++) {
                buffer[(r - 1) * NUM_COLS + c] = buffer[r * NUM_COLS + c];
            }
        }
        // Clear the last row after scrolling
        clear_row(NUM_ROWS - 1);
    }
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (character == '\b') {  // Handle backspace
        if (col == 0 && row > 0) {
            // Move to end of previous row if at start of current row
            row--;
            col = NUM_COLS - 1;
        } else if (col > 0) {
            // Move one position back in the current row
            col--;
        }

        // Clear the character at the current position
        buffer[col + NUM_COLS * row] = (struct Char) {
            character: ' ',
            color: color,
        };
        return;
    }

    if (col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
}


void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

// Read input (mocked for demonstration purposes)
void read_input(char* buffer, size_t buffer_size) {
    size_t i = 0;
    char c;
    while (i < buffer_size - 1) {
        c = get_char();  // Fetch a character from input

        if (c == '\n' || c == '\r') break;  // End input on Enter key

        if (c == '\b') {  // Handle backspace
            if (i > 0) {
                i--;                    // Move buffer index back
                print_char('\b');       // Remove last character from screen
                buffer[i] = '\0';       // Null-terminate the updated buffer
            }
        } else {
            buffer[i++] = c;           // Store character in buffer
            print_char(c);             // Echo character to the screen
        }
    }
    buffer[i] = '\0';  // Final null-terminate the input string
}


int strncmp(const char* str1, const char* str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (str1[i] != str2[i] || str1[i] == '\0') {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
    }
    return 0;
}