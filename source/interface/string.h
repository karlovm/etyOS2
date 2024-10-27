#include <stdint.h>
#ifndef STRING_H
#define STRING_H
int strcmp(const char* str1, const char* str2);

char* strcpy(char* dest, const char* src);
int strlen(const char* str);

void print_hex_digit(unsigned char digit);
void print_hex(unsigned int number);
void print_dec(unsigned int number);
char *strchr(const char *str, int c);
uint32_t strtoul(const char *str, char **endptr, int base);

#endif // STRING_H
