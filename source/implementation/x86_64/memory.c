// memory.c
#include "memory.h"

void* memory_set(void* dest, int value, size_t length) {
    unsigned char* ptr = (unsigned char*)dest;
    while (length-- > 0) {
        *ptr++ = (unsigned char)value;
    }
    return dest;
}

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

void memory_zero(void* dest, size_t length) {
    memory_set(dest, 0, length);
}

int memory_is_zero(const void* ptr, size_t length) {
    const unsigned char* p = (const unsigned char*)ptr;
    while (length-- > 0) {
        if (*p++ != 0) {
            return 0;
        }
    }
    return 1;
}

void memory_swap(void* ptr1, void* ptr2, size_t length) {
    unsigned char* p1 = (unsigned char*)ptr1;
    unsigned char* p2 = (unsigned char*)ptr2;
    
    while (length-- > 0) {
        unsigned char temp = *p1;
        *p1++ = *p2;
        *p2++ = temp;
    }
}

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
