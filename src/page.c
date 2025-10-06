#include <stddef.h>
#include "page.h"

// Static array of 128 pages, each 2mb in length covers 256 megs of memory
struct ppage physical_page_array[128];

// Pointer to the head of the free physical pages list
static struct ppage *free_physical_pages_head = NULL;

// Initialize the linked list of free pages - #4
void init_pfa_list(void) {
    int i;
    
    // Initialize the first page
    physical_page_array[0].next = &physical_page_array[1];
    physical_page_array[0].prev = NULL;
    physical_page_array[0].physical_addr = (void*)(0x100000); // Start at 1MB
    
    // Initialize middle pages
    for (i = 1; i < 127; i++) {
        physical_page_array[i].next = &physical_page_array[i + 1];
        physical_page_array[i].prev = &physical_page_array[i - 1];
        physical_page_array[i].physical_addr = (void*)(0x100000 + (i * 0x200000)); // 2MB increments
    }
    
    // Initialize the last page
    physical_page_array[127].next = NULL;
    physical_page_array[127].prev = &physical_page_array[126];
    physical_page_array[127].physical_addr = (void*)(0x100000 + (127 * 0x200000));
    
    // Set the head of the free list
    free_physical_pages_head = &physical_page_array[0];
}

// Allocate one or more physical pages from the free list - #5
struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0 || free_physical_pages_head == NULL) {
        return NULL; // Invalid request or no free pages
    }
    
    struct ppage *allocated_list = free_physical_pages_head;
    struct ppage *current = free_physical_pages_head;
    unsigned int count = 0;
    
    // Find npages consecutive pages
    while (current != NULL && count < npages) {
        current = current->next;
        count++;
    }
    
    if (count < npages) {
        return NULL; // Not enough pages available
    }
    
    // Update the free list head
    free_physical_pages_head = current;
    if (current != NULL) {
        current->prev = NULL;
    }
    
    // Terminate the allocated list
    if (allocated_list != NULL) {
        struct ppage *last_allocated = allocated_list;
        for (unsigned int i = 0; i < npages - 1; i++) {
            last_allocated = last_allocated->next;
        }
        last_allocated->next = NULL;
    }
    
    return allocated_list;
}

// Free physical pages back to the free list
void free_physical_pages(struct ppage *ppage_list) {
    if (ppage_list == NULL) {
        return; // Nothing to free
    }
    
    // Find the end of the list being freed
    struct ppage *last = ppage_list;
    while (last->next != NULL) {
        last = last->next;
    }
    
    // Add the freed pages to the front of the free list
    last->next = free_physical_pages_head;
    if (free_physical_pages_head != NULL) {
        free_physical_pages_head->prev = last;
    }
    
    ppage_list->prev = NULL;
    free_physical_pages_head = ppage_list;
}
