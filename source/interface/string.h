#ifndef STRING_H
#define STRING_H

int strcmp(const char* str1, const char* str2);

char* strcpy(char* dest, const char* src);
int strlen(const char* str);

void print_hex_digit(unsigned char digit);
void print_hex(unsigned int number);
void print_dec(unsigned int number);

#endif // STRING_H
