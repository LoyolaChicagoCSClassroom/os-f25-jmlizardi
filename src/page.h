#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr;
};

// Function declarations
void init_pfa_list(void);
struct ppage *allocate_physical_pages(unsigned int npages);
void free_physical_pages(struct ppage *ppage_list);

#endif // PAGE_H