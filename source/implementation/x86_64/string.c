#include "string.h"
#include <stdint.h>
#include <stddef.h>

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

uint32_t strtoul(const char *str, char **endptr, int base) {
    uint32_t result = 0;
    int digit;

    while (*str) {
        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'A' && *str <= 'Z') {
            digit = *str - 'A' + 10;
        } else if (*str >= 'a' && *str <= 'z') {
            digit = *str - 'a' + 10;
        } else {
            break;  // Invalid character for the base
        }

        if (digit >= base) {
            break;  // Invalid digit for the base
        }

        result = result * base + digit;
        str++;
    }

    if (endptr) {
        *endptr = (char *)str;
    }

    return result;
}

char *strchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return 0;
}


int strncmp(const char* str1, const char* str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (str1[i] != str2[i] || str1[i] == '\0') {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
    }
    return 0;
}