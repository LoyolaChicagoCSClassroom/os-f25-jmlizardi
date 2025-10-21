#include "mmu.h"
#include "rprintf.h"
#include "terminal.h"
#include <stddef.h>

// Global page directory and page table - must be 4KB aligned
struct page_directory_entry pd[1024] __attribute__((aligned(4096)));
struct page pt[1024] __attribute__((aligned(4096)));

// External symbols from linker script
extern char _end_kernel;

/**
 * Maps a list of physical pages to a specified virtual address
 * 
 * @param vaddr: The virtual address to map pages to
 * @param pglist: Linked list of physical page structures from page allocator
 * @param pd: Pointer to the page directory
 * @return: The virtual address that was mapped
 */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *page_dir) {
    if (!vaddr || !pglist || !page_dir) {
        return NULL;
    }
    
    uint32_t virtual_addr = (uint32_t)vaddr;
    struct ppage *current_page = pglist;
    
    while (current_page != NULL) {
        // Extract page directory index (bits 31-22) and page table index (bits 21-12)
        uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;  // Top 10 bits
        uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;  // Next 10 bits
        
        // For this assignment, we assume we're only mapping to the first page table
        // In a full implementation, we'd need multiple page tables
        if (pd_index != 0) {
            esp_printf((func_ptr)putc, "Warning: Virtual address 0x%08x maps to page directory index %d\n", 
                       virtual_addr, pd_index);
            esp_printf((func_ptr)putc, "This implementation only supports the first page table (index 0)\n");
        }
        
        // Check if page directory entry exists for index 0
        if (!page_dir[0].present) {
            // Set up page directory entry to point to our page table
            page_dir[0].present = 1;
            page_dir[0].rw = 1;      // Read/write
            page_dir[0].user = 0;    // Supervisor only
            page_dir[0].writethru = 0;
            page_dir[0].cachedisabled = 0;
            page_dir[0].accessed = 0;
            page_dir[0].pagesize = 0; // 4KB pages
            page_dir[0].ignored = 0;
            page_dir[0].os_specific = 0;
            page_dir[0].frame = ((uint32_t)pt) >> 12; // Physical address of page table
        }
        
        // Set up page table entry
        pt[pt_index].present = 1;
        pt[pt_index].rw = 1;        // Read/write
        pt[pt_index].user = 0;      // Supervisor only  
        pt[pt_index].accessed = 0;
        pt[pt_index].dirty = 0;
        pt[pt_index].unused = 0;
        pt[pt_index].frame = ((uint32_t)current_page->physical_addr) >> 12; // Physical page frame
        
        // Move to next page and next virtual address (4KB increment)
        current_page = current_page->next;
        virtual_addr += 4096;
    }
    
    return vaddr;
}

/**
 * Load page directory into CR3 register
 */
void loadPageDirectory(struct page_directory_entry *page_dir) {
    asm volatile("mov %0,%%cr3"
        :
        : "r"(page_dir)
        : "memory");
}

/**
 * Enable paging by setting CR0 bits 0 and 31
 */
void enable_paging(void) {
    asm volatile(
        "mov %%cr0, %%eax\n"
        "or $0x80000001,%%eax\n"  // Set bits 0 (PE) and 31 (PG)
        "mov %%eax,%%cr0"
        :
        :
        : "eax", "memory");
}

/**
 * Identity map the kernel, stack, and video buffer
 */
void identity_map_kernel(struct page_directory_entry *page_dir) {
    // Initialize page directory and page table to zero
    for (int i = 0; i < 1024; i++) {
        page_dir[i].present = 0;
        pt[i].present = 0;
    }
    
    // 1. Identity map kernel from 0x100000 to &_end_kernel
    uint32_t kernel_start = 0x100000;  // 1MB
    uint32_t kernel_end = (uint32_t)&_end_kernel;
    
    // Round up to next page boundary
    kernel_end = (kernel_end + 4095) & ~4095;
    
    esp_printf((func_ptr)putc, "Identity mapping kernel: 0x%08x to 0x%08x\n", 
               kernel_start, kernel_end);
    
    // Map kernel pages in 4KB chunks
    for (uint32_t addr = kernel_start; addr < kernel_end; addr += 4096) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void*)addr;
        map_pages((void*)addr, &tmp, page_dir);
    }
    
    // 2. Identity map stack memory
    uint32_t esp;
    asm("mov %%esp,%0" : "=r" (esp));
    
    // Map a few pages around the stack pointer (4KB pages)
    uint32_t stack_base = esp & ~0xFFF;  // Align to page boundary
    esp_printf((func_ptr)putc, "Identity mapping stack around: 0x%08x\n", stack_base);
    
    // Map 8 pages for stack (32KB total) - going downward from current stack
    for (int i = 0; i < 8; i++) {
        uint32_t stack_page = stack_base - (i * 4096);
        struct ppage tmp;
        tmp.next = NULL;
        tmp.prev = NULL;
        tmp.physical_addr = (void*)stack_page;
        map_pages((void*)stack_page, &tmp, page_dir);
    }
    
    // 3. Identity map video buffer at 0xB8000
    esp_printf((func_ptr)putc, "Identity mapping video buffer: 0x000B8000\n");
    struct ppage tmp;
    tmp.next = NULL;
    tmp.prev = NULL;
    tmp.physical_addr = (void*)0xB8000;
    map_pages((void*)0xB8000, &tmp, page_dir);
    
    esp_printf((func_ptr)putc, "Identity mapping complete.\n");
}
