#include <stdint.h>
#include "terminal.h"
#include "rprintf.h"
#include "interrupt.h"
#include "io.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6



const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) =
 { 0xE85250D6, 0, 24, (unsigned)(0 - (0xE85250D6u + 0u + 24u)), 0, 8 };
 //had to modify this from original kernel_main.c... OS was permantly rebooting

/*uint8_t inb (uint16_t _port) { //already included with kernel
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}*/

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
// Remap PIC and enable keyboard IRQ
    //remap_pic();
    //IRQ_clear_mask(1); // Unmask keyboard IRQ
    //init_idt(); // Initialize IDT (if not already done)

    // Main loop can be empty or used for other tasks; keyboard input is now interrupt-driven
    while (1) {
        // Idle loop
    }

 }

