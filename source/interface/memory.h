// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Fills a block of memory with a specified value
void* memory_set(void* dest, int value, size_t length);

// Copies bytes from source to destination
void* memory_copy(void* dest, const void* src, size_t length);

// Compares two blocks of memory
int memory_compare(const void* ptr1, const void* ptr2, size_t length);

// Searches for a byte value in a block of memory
void* memory_find(const void* ptr, int value, size_t length);

// Zero out a block of memory
void memory_zero(void* dest, size_t length);

// Check if memory region contains only zeros
int memory_is_zero(const void* ptr, size_t length);

// Swap contents of two memory regions
void memory_swap(void* ptr1, void* ptr2, size_t length);

// Move a block of memory with proper overlap handling
void* memory_move(void* dest, const void* src, size_t length);

#endif // MEMORY_H
