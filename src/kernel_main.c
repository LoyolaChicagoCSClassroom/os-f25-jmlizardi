#include <stdint.h>
#include "terminal.h"
#include "rprintf.h"
#include "interrupt.h"
#include "io.h"
#include "fat.h"
#include "malloc.h"

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) =
 { 0xE85250D6, 0, 24, (unsigned)(0 - (0xE85250D6u + 0u + 24u)), 0, 8 };

// Keyboard scancode to ASCII lookup table
unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
 '9', '0', '-', '=', '\b',
 '\t',
 'q', 'w', 'e', 'r',
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
   0,
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
'\'', '`',   0,
'\\', 'z', 'x', 'c', 'v', 'b', 'n',
 'm', ',', '.', '/',   0,
 '*',
   0,
 ' ',
   0,
   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,
   0,
   0,
   0,
   0,
 '-',
   0,
   0,
   0,
 '+',
   0,
   0,
   0,
   0,
   0,
   0,   0,   0,
   0,
   0,
   0,
   0,   0,   0,
   0,
   0,
   0,   0,   0,   0,   0,   0,   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
};

// Video memory structure
struct video_buf {
    unsigned char character;
    unsigned char color;
};

int main(void) {
    // Initialize video memory
    struct video_buf *vram = (struct video_buf*)0xb8000;
    
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vram[i].character = ' ';
        vram[i].color = 0x07;
    }
    
    // Initialize terminal
    terminal_init();
    
    // Initialize interrupts
    interrupt_init();
    
    // Print welcome message
    esp_printf((func_ptr)putc, "OS Assignment 5 - FAT File System\n");
    esp_printf((func_ptr)putc, "===================================\n\n");
    
    // Initialize FAT filesystem
    esp_printf((func_ptr)putc, "Initializing FAT filesystem...\n");
    if (fat_init() != 0) {
        esp_printf((func_ptr)putc, "ERROR: Failed to initialize FAT filesystem!\n");
        esp_printf((func_ptr)putc, "Boot sector may be invalid or disk not accessible.\n");
    } else {
        esp_printf((func_ptr)putc, "FAT filesystem initialized successfully!\n\n");
        
        // List files in root directory
        esp_printf((func_ptr)putc, "Files in root directory:\n");
        esp_printf((func_ptr)putc, "------------------------\n");
        fat_list_files();
        esp_printf((func_ptr)putc, "\n");
        
        // Try to open and read the kernel file
        esp_printf((func_ptr)putc, "Attempting to read KERNEL file...\n");
        struct file* kernel_file = fat_open("KERNEL");
        if (kernel_file) {
            esp_printf((func_ptr)putc, "Successfully opened KERNEL file\n");
            esp_printf((func_ptr)putc, "File size: %d bytes\n", kernel_file->rde.file_size);
            
            // Read first 128 bytes as a test
            unsigned char buffer[128];
            int bytes_read = fat_read(kernel_file, buffer, 128, 0);
            esp_printf((func_ptr)putc, "Read %d bytes from file\n", bytes_read);
            
            // Display first 16 bytes in hex
            esp_printf((func_ptr)putc, "First 16 bytes (hex): ");
            for (int i = 0; i < 16 && i < bytes_read; i++) {
                esp_printf((func_ptr)putc, "%02x ", buffer[i]);
            }
            esp_printf((func_ptr)putc, "\n");
            
            fat_close(kernel_file);
        } else {
            esp_printf((func_ptr)putc, "Failed to open KERNEL file\n");
        }
    }
    
    esp_printf((func_ptr)putc, "\nMemory usage:\n");
    esp_printf((func_ptr)putc, "Heap used: %d bytes\n", get_heap_used());
    esp_printf((func_ptr)putc, "Heap free: %d bytes\n\n", get_heap_free());
    
    esp_printf((func_ptr)putc, "Press any key to continue...\n");
    
    // Simple keyboard input loop
    while (1) {
        unsigned char status = inb(0x64);
        if (status & 0x01) {
            unsigned char scancode = inb(0x60);
            
            // Check if key released (bit 7 set means release)
            if (!(scancode & 0x80)) {
                char ascii = keyboard_map[scancode];
                if (ascii) {
                    putc(ascii);
                }
            }
        }
    }
    
    return 0;
}
