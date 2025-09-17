

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>


#define PIC_EOI		0x20		 //  End-of-interrupt command code
#define PIC1		0x20         // IO base address for master PIC
#define PIC2		0xA0         // IO base address for slave PIC
#define PIC_1_COMMAND	PIC1
//#define PIC1_DATA	(PIC1+1)
#define PIC_2_COMMAND	PIC2
//#define PIC2_DATA	(PIC2+1)

#define IDT_SIZE 256
#define PIC_1_CTRL 0x20
#define PIC_2_CTRL 0xA0
#define PIC_1_DATA 0x21
#define PIC_2_DATA 0xA1


// A struct describing an interrupt gate.
struct idt_entry
{
   uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   uint16_t sel;                 // Kernel segment selector.
   uint8_t  always0;             // This must always be zero.
   uint8_t  flags;               // More flags. See documentation.
   uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));


// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr
{
   uint16_t limit;
   uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));


struct eflags {
    unsigned int carry       : 1;
    unsigned int reserved1   : 1; // Always 1
    unsigned int parity      : 1;
    unsigned int reserved3   : 1;
    unsigned int adjust      : 1;
    unsigned int reserved5   : 1;
    unsigned int zero        : 1;
    unsigned int sign        : 1;
    unsigned int trap        : 1;
    unsigned int interrupt   : 1;
    unsigned int direction   : 1;
    unsigned int overflow    : 1;
    unsigned int iopl        : 2;
    unsigned int nest        : 1;
    unsigned int reserved15  : 1;
    unsigned int resume      : 1;
    unsigned int v8086       : 1;
    unsigned int reserved18  : 14;
}__attribute__((packed));

struct interrupt_frame_with_error {
    uint32_t errcode;
    uint32_t eip;
    uint16_t cs;
    uint16_t unused1;
    struct  eflags eflags;
    uint32_t esp;
    uint16_t ss;
    uint16_t unused2;
}__attribute__((packed));


struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint16_t unused1;
    struct  eflags eflags;
    uint32_t esp;
    uint16_t ss;
    uint16_t unused2;
}__attribute__((packed));



struct seg_desc{
    uint16_t sz;
    uint32_t addr;
} __attribute__((packed));


// A struct describing a Task State Segment.
struct tss_entry {
   uint32_t prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
   uint32_t esp0;       // The stack pointer to load when we change to kernel mode.
   uint32_t ss0;        // The stack segment to load when we change to kernel mode.
   uint32_t esp1;       // everything below here is unusued now.. 
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;         
   uint32_t cs;        
   uint32_t ss;        
   uint32_t ds;        
   uint32_t fs;       
   uint32_t gs;         
   uint32_t ldt;      
   uint16_t trap;
   uint16_t iomap_base;
}__attribute__((packed));

struct gdt_entry_bits
{
	unsigned int limit_low:16;
	unsigned int base_low : 24;
     //attribute byte split into bitfields
	unsigned int accessed :1;
	unsigned int read_write :1; //readable for code, writable for data
	unsigned int conforming_expand_down :1; //conforming for code, expand down for data
	unsigned int code :1; //1 for code, 0 for data
	unsigned int always_1 :1; //should be 1 for everything but TSS and LDT
	unsigned int DPL :2; //priviledge level
	unsigned int present :1;
     //and now into granularity
	unsigned int limit_high :4;
	unsigned int available :1;
	unsigned int always_0 :1; //should always be 0
	unsigned int big :1; //32bit opcodes for code, uint32_t stack for data
	unsigned int gran :1; //1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high :8;
} __attribute__((packed));





void PIC_sendEOI(unsigned char irq);
void IRQ_clear_mask(unsigned char IRQline);
void IRQ_set_mask(unsigned char IRQline);
void init_idt();
void tss_flush (uint16_t tss);
void load_gdt();
void remap_pic(void);
#endif
