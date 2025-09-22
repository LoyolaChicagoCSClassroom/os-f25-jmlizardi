#include <stdint.h>
#include "terminal.h"
#include "rprintf.h"
#include "interrupt.h"
#include "io.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6



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
// Remap PIC and enable keyboard IRQ *NOT IN USE KEEPING IT TO MAKE INTERRUPTS WORK LATER*
    //remap_pic();
    //IRQ_clear_mask(1); // Unmask keyboard IRQ
    //init_idt(); // Initialize IDT (if not already done)

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

    // Polling-based keyboard input with translation
    esp_printf((func_ptr)putc, "\nKeyboard ready - start typing:\n");
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
                // Print both scancode and translated character
                esp_printf((func_ptr)putc, "Scancode: 0x%02x -> '%c'\n", scancode, ascii);
            }
        }
    }

 }

