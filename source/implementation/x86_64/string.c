#include "string.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strcpy(char* dest, const char* src) {
    char* start = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return start;
}

int strlen(const char* str) {
    int length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

// Implementation for print.c
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