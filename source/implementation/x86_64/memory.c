#include "memory.h"

/**
 * memory_set - Fills a block of memory with a specified value.
 * 
 * @param dest: Pointer to the memory block to fill.
 * @param value: Value to set each byte to.
 * @param length: Number of bytes to set.
 * 
 * This function fills the memory block starting at 'dest' with the 
 * byte value 'value'. It iterates over the block and assigns each 
 * byte the same value, effectively initializing or resetting a 
 * memory region.
 * 
 * @return: Returns the pointer to the destination memory block 'dest'.
 */
void* memory_set(void* dest, int value, size_t length) {
    unsigned char* ptr = (unsigned char*)dest;
    while (length-- > 0) {
        *ptr++ = (unsigned char)value;
    }
    return dest;
}

/**
 * memory_copy - Copies a block of memory from source to destination.
 * 
 * @param dest: Pointer to the destination memory block.
 * @param src: Pointer to the source memory block.
 * @param length: Number of bytes to copy.
 * 
 * This function copies 'length' bytes from the memory block pointed 
 * to by 'src' to the memory block pointed to by 'dest'. It handles 
 * overlapping regions by deciding whether to copy forward or backward 
 * to ensure that the copied data remains correct.
 * 
 * @return: Returns the pointer to the destination memory block 'dest'.
 */
void* memory_copy(void* dest, const void* src, size_t length) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    // Handle overlapping memory regions
    if (d < s) {
        // Copy forward
        while (length-- > 0) {
            *d++ = *s++;
        }
    } else if (d > s) {
        // Copy backward to handle overlapping regions
        d += length;
        s += length;
        while (length-- > 0) {
            *--d = *--s;
        }
    }
    
    return dest;
}

/**
 * memory_compare - Compares two blocks of memory.
 * 
 * @param ptr1: Pointer to the first memory block.
 * @param ptr2: Pointer to the second memory block.
 * @param length: Number of bytes to compare.
 * 
 * This function compares 'length' bytes between the two memory blocks 
 * pointed to by 'ptr1' and 'ptr2'. It returns 0 if the memory blocks 
 * are identical, 1 if 'ptr1' is greater than 'ptr2', and -1 if 'ptr1' 
 * is less than 'ptr2'.
 * 
 * @return: Returns an integer indicating the comparison result.
 */
int memory_compare(const void* ptr1, const void* ptr2, size_t length) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    while (length-- > 0) {
        if (*p1 != *p2) {
            return (*p1 > *p2) ? 1 : -1;
        }
        p1++;
        p2++;
    }
    return 0;
}

/**
 * memory_find - Searches for a specific value within a block of memory.
 * 
 * @param ptr: Pointer to the memory block to search.
 * @param value: The value to search for (as an integer).
 * @param length: Number of bytes to search.
 * 
 * This function searches the memory block pointed to by 'ptr' for the 
 * first occurrence of the byte value 'value'. If found, it returns a 
 * pointer to the first occurrence of the value. If not found, it 
 * returns NULL.
 * 
 * @return: Returns a pointer to the first occurrence of the value, 
 * or NULL if not found.
 */
void* memory_find(const void* ptr, int value, size_t length) {
    const unsigned char* p = (const unsigned char*)ptr;
    unsigned char v = (unsigned char)value;
    
    while (length-- > 0) {
        if (*p == v) {
            return (void*)p;
        }
        p++;
    }
    return NULL;
}

/**
 * memory_zero - Fills a block of memory with zeros.
 * 
 * @param dest: Pointer to the memory block to zero out.
 * @param length: Number of bytes to zero out.
 * 
 * This function calls 'memory_set' to fill the memory block starting 
 * at 'dest' with zeros. It's a convenience function used to clear 
 * memory or reset a region.
 */
void memory_zero(void* dest, size_t length) {
    memory_set(dest, 0, length);
}

/**
 * memory_is_zero - Checks if a memory block is filled with zeros.
 * 
 * @param ptr: Pointer to the memory block.
 * @param length: Number of bytes to check.
 * 
 * This function checks whether all 'length' bytes in the memory block 
 * pointed to by 'ptr' are zeros. It returns 1 if all bytes are zero, 
 * and 0 otherwise.
 * 
 * @return: Returns 1 if all bytes are zero, 0 otherwise.
 */
int memory_is_zero(const void* ptr, size_t length) {
    const unsigned char* p = (const unsigned char*)ptr;
    while (length-- > 0) {
        if (*p++ != 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * memory_swap - Swaps two blocks of memory of the same size.
 * 
 * @param ptr1: Pointer to the first memory block.
 * @param ptr2: Pointer to the second memory block.
 * @param length: Number of bytes to swap.
 * 
 * This function swaps the contents of two memory blocks pointed to 
 * by 'ptr1' and 'ptr2', byte by byte. The size of both memory blocks 
 * should be equal for the swap to work correctly.
 */
void memory_swap(void* ptr1, void* ptr2, size_t length) {
    unsigned char* p1 = (unsigned char*)ptr1;
    unsigned char* p2 = (unsigned char*)ptr2;
    
    while (length-- > 0) {
        unsigned char temp = *p1;
        *p1++ = *p2;
        *p2++ = temp;
    }
}

/**
 * memory_move - Safely moves a block of memory.
 * 
 * @param dest: Pointer to the destination memory block.
 * @param src: Pointer to the source memory block.
 * @param length: Number of bytes to move.
 * 
 * This function moves 'length' bytes from the memory block pointed to 
 * by 'src' to the memory block pointed to by 'dest'. It handles overlapping 
 * regions similarly to 'memory_copy', copying forward or backward based on 
 * the relative positions of 'dest' and 'src'.
 * 
 * @return: Returns the pointer to the destination memory block 'dest'.
 */
void* memory_move(void* dest, const void* src, size_t length) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    if (d < s) {
        // Copy forward
        while (length-- > 0) {
            *d++ = *s++;
        }
    } else if (d > s) {
        // Copy backward to handle overlapping regions
        d += length;
        s += length;
        while (length-- > 0) {
            *--d = *--s;
        }
    }
    
    return dest;
}
