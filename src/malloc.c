#include <stddef.h>
#include <stdint.h>

// Simple heap allocator for the kernel
#define HEAP_SIZE 8192
static char heap[HEAP_SIZE];
static char* heap_ptr = heap;

void* malloc(size_t size) {
    // Align to 4-byte boundary
    size = (size + 3) & ~3;
    
    if (heap_ptr + size > heap + HEAP_SIZE) {
        return NULL; // Out of memory
    }
    
    void* ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    // Simple implementation - no actual freeing for now
    // In a real implementation, you'd track allocated blocks
    (void)ptr;
}

// Get heap usage information
size_t get_heap_used(void) {
    return heap_ptr - heap;
}

size_t get_heap_free(void) {
    return HEAP_SIZE - (heap_ptr - heap);
}
