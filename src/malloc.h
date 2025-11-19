#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <stddef.h>

void* malloc(size_t size);
void free(void* ptr);
size_t get_heap_used(void);
size_t get_heap_free(void);

#endif
