#include <stdint.h>
#include "terminal.h"
#include "rprintf.h"
#include "interrupt.h"
#include "io.h"
#include "page.h"
#include "mmu.h"  // Re-enabled for step-by-step testing

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) =
 { 0xE85250D6, 0, 24, (unsigned)(0 - (0xE85250D6u + 0u + 24u)), 0, 8 };
 //had to modify this from original kernel_main.c... OS was permantly rebooting

// Keyboard scancode to ASCII lookup table
unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
 '9', '0', '-', '=', '\b',     /* Backspace */
 '\t',                 /* Tab */
 'q', 'w', 'e', 'r',   /* 19 */
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
   0,                  /* 29   - Control */
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 39 */
'\'', '`',   0,                /* Left shift */
'\\', 'z', 'x', 'c', 'v', 'b', 'n',                    /* 49 */
 'm', ',', '.', '/',   0,                              /* Right shift */
 '*',
   0,  /* Alt */
 ' ',  /* Space bar */
   0,  /* Caps lock */
   0,  /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,  
   0,  /* < ... F10 */
   0,  /* 69 - Num lock*/
   0,  /* Scroll Lock */
   0,  /* Home key */
   0,  /* Up Arrow */
   0,  /* Page Up */
 '-',
   0,  /* Left Arrow */
   0,  
   0,  /* Right Arrow */
 '+',
   0,  /* 79 - End key*/
   0,  /* Down Arrow */
   0,  /* Page Down */
   0,  /* Insert Key */
   0,  /* Delete Key */
   0,   0,   0,  
   0,  /* F11 Key */
   0,  /* F12 Key */
   0,  /* All other keys are undefined */
};

 void main() {
    struct video_buf *vram = (struct video_buf*)0xb8000; // Base address of video mem
    const unsigned char color = 7; // gray text on black background

    vram[0].ascii = 'a'; //baremetal way of printing to first cell
    vram[0].color = 7; // color

    vram[1].ascii = 'b'; //baremetal way of printing to second cell
    vram[1].color = 7; //color
// Conducts CPL check to later be called when printing execution level
 int get_cpl(void) {
    unsigned short cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    return cs & 0x3;
}

    // Print current execution level
    void print_execution_level() { // makes a easily callable method to print cpl
	int ring = get_cpl(); // current privilge level
       esp_printf((func_ptr)putc, "Current execution level is ring %d\n", ring);
    }
// prints hello one time using terminal driver and then exits with break
     while (1) {
       putc('H');
       putc('E');
       putc('L');
       putc('L');
       putc('0');
       putc('\n');
       break;
     }

     int line_number = 0;
     while(line_number < 35) {
     esp_printf((func_ptr)putc, "Line %d: Justin Was Here!\n", line_number++);
     }
     print_execution_level(); // After the print executes, showing it works & scrolls print CPL 

    // Test the page frame allocator
    esp_printf((func_ptr)putc, "\n=== Testing Page Frame Allocator ===\n");
    
    // Initialize the page allocator
    init_pfa_list();
    esp_printf((func_ptr)putc, "Page allocator initialized.\n");
    
    // Allocate 2 pages
    struct ppage *allocated_pages = allocate_physical_pages(2);
    if (allocated_pages != NULL) {
        esp_printf((func_ptr)putc, "Successfully allocated 2 pages starting at: 0x%08x\n", 
                   (unsigned int)allocated_pages->physical_addr);
    } else {
        esp_printf((func_ptr)putc, "Failed to allocate 2 pages\n");
    }
    
    // Free the pages
    if (allocated_pages != NULL) {
        free_physical_pages(allocated_pages);
        esp_printf((func_ptr)putc, "Freed 2 pages back to the allocator.\n");
    }
    
    esp_printf((func_ptr)putc, "Page allocator test complete.\n\n");

    // === Assignment #4: MMU Testing - Step 1 ===
    esp_printf((func_ptr)putc, "=== Step 1: Testing MMU structure access ===\n");
    
    // Test 1: Just access the page directory structure
    extern struct page_directory_entry pd[1024];
    esp_printf((func_ptr)putc, "Page directory address: 0x%08x\n", (unsigned int)pd);
    esp_printf((func_ptr)putc, "Page directory size: %d bytes\n", sizeof(pd));
    
    // Test 2: Access page table structure  
    extern struct page pt[1024];
    esp_printf((func_ptr)putc, "Page table address: 0x%08x\n", (unsigned int)pt);
    esp_printf((func_ptr)putc, "Page table size: %d bytes\n", sizeof(pt));
    
    // Test 3: Initialize a few entries manually (no function calls yet)
    esp_printf((func_ptr)putc, "Manually initializing first page directory entry...\n");
    pd[0].present = 0;  // Just set to 0 for now
    pd[0].rw = 0;
    esp_printf((func_ptr)putc, "Page directory entry initialized successfully.\n");
    
    esp_printf((func_ptr)putc, "MMU structure access test complete - no crashes!\n\n");

    // === Assignment #4: MMU Testing - Step 2 ===
    esp_printf((func_ptr)putc, "=== Step 2: Testing identity_map_kernel function ===\n");
    
    // Test the identity mapping function
    esp_printf((func_ptr)putc, "Calling identity_map_kernel()...\n");
    identity_map_kernel(pd);
    esp_printf((func_ptr)putc, "identity_map_kernel() completed successfully!\n");
    
    // Check if the page directory was set up
    if (pd[0].present) {
        esp_printf((func_ptr)putc, "Page directory entry 0 is present - good!\n");
        esp_printf((func_ptr)putc, "PD[0]: present=%d, rw=%d, frame=0x%x\n", 
                   pd[0].present, pd[0].rw, pd[0].frame);
    } else {
        esp_printf((func_ptr)putc, "Warning: Page directory entry 0 is not present\n");
    }
    
    esp_printf((func_ptr)putc, "Identity mapping test complete - no crashes!\n\n");

    // === Assignment #4: MMU Testing - Step 3 ===
    esp_printf((func_ptr)putc, "=== Step 3: Testing loadPageDirectory function ===\n");
    
    // Test loading the page directory into CR3 (without enabling paging)
    esp_printf((func_ptr)putc, "About to call loadPageDirectory()...\n");
    loadPageDirectory(pd);
    esp_printf((func_ptr)putc, "loadPageDirectory() completed successfully!\n");
    esp_printf((func_ptr)putc, "Page directory loaded into CR3 register.\n");
    esp_printf((func_ptr)putc, "Note: Paging is NOT enabled yet, so this should be safe.\n\n");

    // === Assignment #4: MMU Testing - Step 4 (FINAL TEST) ===
    esp_printf((func_ptr)putc, "=== Step 4: Testing enable_paging function (THE MOMENT OF TRUTH) ===\n");
    
    esp_printf((func_ptr)putc, "About to enable paging - this is where it might crash...\n");
    esp_printf((func_ptr)putc, "Fixed: Only setting CR0 bit 31 (PG), not touching bit 0 (PE).\n");
    
    enable_paging();
    
    esp_printf((func_ptr)putc, "*** SUCCESS! *** Paging is now enabled!\n");
    esp_printf((func_ptr)putc, "Virtual memory is active and working!\n");
    esp_printf((func_ptr)putc, "All memory accesses are going through the MMU.\n\n");

    // Test that memory mapping works by allocating and mapping some pages
    esp_printf((func_ptr)putc, "=== Page allocator test (without MMU functions) ===\n");
    struct ppage *test_pages = allocate_physical_pages(1);
    if (test_pages != NULL) {
        esp_printf((func_ptr)putc, "Allocated test page at physical address: 0x%08x\n", 
                   (unsigned int)test_pages->physical_addr);
        free_physical_pages(test_pages);
        esp_printf((func_ptr)putc, "Test page freed successfully.\n");
    }
    esp_printf((func_ptr)putc, "Page allocator working correctly without MMU.\n\n");

    // Interactive keyboard commands for page allocator
    // Implemented to control page allocation via keyboard for demo purposes
    esp_printf((func_ptr)putc, "\nPage Allocator Commands:\n");
    esp_printf((func_ptr)putc, "Press '1' to allocate 1 page\n");
    esp_printf((func_ptr)putc, "Press '2' to allocate 2 pages\n");
    esp_printf((func_ptr)putc, "Press 'f' to free all allocated pages\n");
    esp_printf((func_ptr)putc, "Press 's' to show allocator status\n");
    esp_printf((func_ptr)putc, "Other keys will show scancode\n\n");
    
    // Track allocated pages for interactive demo
    static struct ppage *demo_allocated_pages = NULL;
    
    while (1) {
        // Check if keyboard has data
        unsigned char status = inb(0x64);
        if (status & 0x01) {  // Output buffer full
            unsigned char scancode = inb(0x60);
            
            // Handle key release (high bit set) - ignore these
            if (scancode & 0x80) {
                continue; // Key release, ignore
            }
            
            // Translate scancode to ASCII using the lookup table
            char ascii = keyboard_map[scancode];
            
            if (ascii != 0) {
                // Handle page allocator commands
                // Used CoPilot to generate this section, utilized for testing page allocator
                if (ascii == '1') {
                    esp_printf((func_ptr)putc, "Allocating 1 page...\n");
                    struct ppage *pages = allocate_physical_pages(1);
                    if (pages) {
                        esp_printf((func_ptr)putc, "Success! Allocated page at 0x%08x\n", 
                                   (unsigned int)pages->physical_addr);
                        // Link to our demo list
                        pages->next = demo_allocated_pages;
                        if (demo_allocated_pages) demo_allocated_pages->prev = pages;
                        demo_allocated_pages = pages;
                    } else {
                        esp_printf((func_ptr)putc, "Failed to allocate page\n");
                    }
                } else if (ascii == '2') {
                    esp_printf((func_ptr)putc, "Allocating 2 pages...\n");
                    struct ppage *pages = allocate_physical_pages(2);
                    if (pages) {
                        esp_printf((func_ptr)putc, "Success! Allocated 2 pages starting at 0x%08x\n", 
                                   (unsigned int)pages->physical_addr);
                        // Link to our demo list
                        pages->next = demo_allocated_pages;
                        if (demo_allocated_pages) demo_allocated_pages->prev = pages;
                        demo_allocated_pages = pages;
                    } else {
                        esp_printf((func_ptr)putc, "Failed to allocate 2 pages\n");
                    }
                } else if (ascii == 'f' || ascii == 'F') {
                    if (demo_allocated_pages) {
                        esp_printf((func_ptr)putc, "Freeing all allocated pages...\n");
                        free_physical_pages(demo_allocated_pages);
                        demo_allocated_pages = NULL;
                        esp_printf((func_ptr)putc, "All pages freed!\n");
                    } else {
                        esp_printf((func_ptr)putc, "No pages to free\n");
                    }
                } else if (ascii == 's' || ascii == 'S') {
                    esp_printf((func_ptr)putc, "Page allocator status:\n");
                    // Count free pages by walking the free list
                    // (This would require adding a status function to page.c)
                    esp_printf((func_ptr)putc, "Demo allocated pages: %s\n", 
                               demo_allocated_pages ? "Yes" : "None");
                } else {
                    // Show scancode for other keys
                    esp_printf((func_ptr)putc, "Key '%c' (scancode: 0x%02x)\n", ascii, scancode);
                }
            }
        }
    }

 }

