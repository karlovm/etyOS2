#include "memory_allocator.h"
#include <stdint.h>
// The size of the memory pool in bytes
#define MEMORY_POOL_SIZE 1024 * 1024 * 100 // 100 MB pool

// Memory pool where the allocations will come from
static uint8_t memory_pool[MEMORY_POOL_SIZE];

// Structure to represent a block of memory
typedef struct MemoryBlock {
    size_t size;
    int is_free;
    struct MemoryBlock* next;
} MemoryBlock;

// Pointer to the first block in the free list
static MemoryBlock* free_list = (MemoryBlock*)memory_pool;

// Initializes the memory allocator
void memory_allocator_init() {
    free_list->size = MEMORY_POOL_SIZE - sizeof(MemoryBlock);
    free_list->is_free = 1;
    free_list->next = NULL;
}

// Allocates a block of memory of the given size
void* kmalloc(size_t size) {
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

// Frees a previously allocated block of memory
void kfree(void* ptr) {
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
