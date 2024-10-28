#include "memory_allocator.h"
#include <stdint.h>

// The size of the memory pool in bytes
#define MEMORY_POOL_SIZE 1024 * 1024 * 100 // 100 MB pool

// Memory pool where the allocations will come from
static uint8_t memory_pool[MEMORY_POOL_SIZE];

// Structure to represent a block of memory
typedef struct MemoryBlock {
    size_t size;          // Size of the memory block
    int is_free;          // Flag indicating whether the block is free
    struct MemoryBlock* next; // Pointer to the next block in the free list
} MemoryBlock;

// Pointer to the first block in the free list
static MemoryBlock* free_list = (MemoryBlock*)memory_pool;

/**
 * memory_allocator_init - Initializes the memory allocator.
 * 
 * This function sets up the memory pool by initializing the free list 
 * with a single large block representing the entire memory pool. 
 * It marks the block as free and available for allocation. 
 * This function should be called before any allocation requests.
 */
void memory_allocator_init() {
    free_list->size = MEMORY_POOL_SIZE - sizeof(MemoryBlock);
    free_list->is_free = 1;
    free_list->next = NULL;
}

/**
 * allocate - Allocates a block of memory of the given size.
 * 
 * @param size: The size (in bytes) of memory to allocate.
 * 
 * This function searches the free list for a memory block large enough 
 * to satisfy the allocation request. If a suitable block is found, 
 * it may split the block if it's too large. It then marks the block 
 * as used (not free) and returns a pointer to the usable memory 
 * within the block. If no suitable block is found, the function returns NULL.
 * 
 * @return: Returns a pointer to the allocated memory, or NULL if 
 * allocation fails.
 */
void* allocate(size_t size) {
    MemoryBlock* current = free_list;

    while (current != NULL) {
        // Find a free block large enough
        if (current->is_free && current->size >= size) {
            // Split the block if it's too large
            if (current->size > size + sizeof(MemoryBlock)) {
                MemoryBlock* new_block = (MemoryBlock*)((uint8_t*)current + sizeof(MemoryBlock) + size);
                new_block->size = current->size - size - sizeof(MemoryBlock);
                new_block->is_free = 1;
                new_block->next = current->next;
                current->next = new_block;
            }

            // Mark the current block as not free and return it
            current->is_free = 0;
            current->size = size;
            return (void*)((uint8_t*)current + sizeof(MemoryBlock));
        }
        current = current->next;
    }

    // No suitable block found
    return NULL;
}

/**
 * free - Frees a previously allocated block of memory.
 * 
 * @param ptr: Pointer to the block of memory to be freed.
 * 
 * This function marks the memory block associated with the given pointer 
 * as free, making it available for future allocations. It also attempts 
 * to coalesce (merge) adjacent free blocks to reduce fragmentation 
 * and make larger contiguous memory blocks available.
 * 
 * @return: None.
 */
void free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // Get the memory block associated with the given pointer
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    block->is_free = 1;

    // Coalesce adjacent free blocks
    MemoryBlock* current = free_list;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(MemoryBlock) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}
