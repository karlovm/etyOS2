#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include <stddef.h>  // for size_t

// Initializes the memory pool
void memory_allocator_init();

// Allocates a block of memory of the given size
void* allocate(size_t size);

// Frees a previously allocated block of memory
void free(void* ptr);

#endif // MEMORY_ALLOCATOR_H
