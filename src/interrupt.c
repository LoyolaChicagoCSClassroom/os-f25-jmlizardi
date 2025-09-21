#include <stdint.h>
#include "interrupt.h"
#include "io.h"
#include "terminal.h"

struct idt_entry idt_entries[256];
struct idt_ptr   idt_ptr;
struct tss_entry tss_ent;


void memset(char *s, char c, unsigned int n) {
    for(int k = 0; k < n ; k++) {
        s[k] = c;
    }
}

void tss_flush (uint16_t tss) {
  asm("ltr %0" : :"a"(tss));
}


struct gdt_entry_bits gdt[] = {{
    .limit_low = 0, // gdt always starts w NULL descriptor
    .base_low = 0,
    .accessed = 0,
    .read_write = 0,
    .conforming_expand_down = 0,
    .code = 0,
    .always_1 = 0,
    .DPL = 0,
    .present = 0,
    .limit_high = 0,
    .available = 0,
    .always_0 = 0,
    .big = 0,
    .gran = 0,
    .base_high = 0
},{   // Kernel code descriptor
    .limit_low = 0xffff,
    .base_low = 0,
    .accessed = 0,
    .read_write = 1,
    .conforming_expand_down = 0,
    .code = 1,
    .always_1 = 1,
    .DPL = 0,
    .present = 1,
    .limit_high = 0xf,
    .available = 0,
    .always_0 = 0,
    .big = 1,
    .gran = 1,
    .base_high = 0
},{ // Kernel data Descriptor
    .limit_low = 0xffff,
    .base_low = 0,
    .accessed = 0,
    .read_write = 1,
    .conforming_expand_down = 0,
    .code = 0,
    .always_1 = 1,
    .DPL = 0,
    .present = 1,
    .limit_high = 0xf,
    .available = 0,
    .always_0 = 0,
    .big = 1,
    .gran = 1,
    .base_high = 0
},{   // User code descriptor
    .limit_low = 0xffff,
    .base_low = 0,
    .accessed = 0,
    .read_write = 1,
    .conforming_expand_down = 0,
    .code = 1,
    .always_1 = 1,
    .DPL = 3,
    .present = 1,
    .limit_high = 0xf,
    .available = 0,
    .always_0 = 0,
    .big = 1,
    .gran = 1,
    .base_high = 0
},{ // User data Descriptor
    .limit_low = 0xffff,
    .base_low = 0,
    .accessed = 0,
    .read_write = 1,
    .conforming_expand_down = 0,
    .code = 0,
    .always_1 = 1,
    .DPL = 3,
    .present = 1,
    .limit_high = 0xf,
    .available = 0,
    .always_0 = 0,
    .big = 1,
    .gran = 1,
    .base_high = 0
},{ // Now, add our TSS descriptor's address to the GDT.
//    .limit_low = sizeof(struct tss_entry) & 0xFFFF,
//    .base_low = (uint32_t)(&tss_ent) & 0xFFFFFF, //isolate bottom 24 bits
    .accessed = 1, //This indicates it's a TSS and not a LDT. This is a changed meaning
    .read_write = 0, //This indicates if the TSS is busy or not. 0 for not busy
    .conforming_expand_down = 0, //always 0 for TSS
    .code = 1, //For TSS this is 1 for 32bit usage, or 0 for 16bit.
    .always_1 = 0, //indicate it is a TSS
    .DPL = 3, //same meaning
    .present = 1, //same meaning
    .limit_high = (sizeof(struct tss_entry) & 0xF0000)>>16, //isolate top nibble
    .available = 0,
    .always_0 = 0, //same thing
    .big = 0, //should leave zero according to manuals. No effect
    .gran = 0, //so that our computed GDT limit is in bytes, not pages
//    .base_high = ((uint32_t)(&tss_ent) & 0xFF000000)>>24, //isolate top byte.
}
};

struct seg_desc gdt_desc = { .sz = sizeof(gdt)-1, .addr = (uint32_t)(&gdt[0]) };


void load_gdt() {


    asm("cli\n"
        "lgdt [gdt_desc]\n"     // Load the new GDT
        "ljmp $0x8,$gdt_flush\n"   // Far jump to update the CS
"gdt_flush:\n"
        "mov %%eax, 0x10\n"       // set data segments to data selector (0x10)
        "mov %%ds, %%eax\n"
        "mov %%ss, %%eax\n"
        "mov %%es, %%eax\n"
        "mov %%fs, %%eax\n"
        "mov %%gs, %%eax\n" : : : "eax");

}



void write_tss(struct gdt_entry_bits *g) {
    // Firstly, let's compute the base and limit of our entry into the GDT.
    uint32_t base = (uint32_t) &tss_ent;
    uint32_t limit = base + sizeof(struct tss_entry);
    extern uint32_t stack_top;

    // Now, add our TSS descriptor's address to the GDT.
    g->limit_low = limit & 0xFFFF;
    g->base_low = base & 0xFFFFFF; //isolate bottom 24 bits
    g->accessed = 1; //This indicates it's a TSS and not a LDT. This is a changed meaning
    g->read_write = 0; //This indicates if the TSS is busy or not. 0 for not busy
    g->conforming_expand_down = 0; //always 0 for TSS
    g->code = 1; //For TSS this is 1 for 32bit usage, or 0 for 16bit.
    g->always_1 = 0; //indicate it is a TSS
    g->DPL = 3; //same meaning
    g->present = 1; //same meaning
    g->limit_high = (limit & 0xF0000)>>16; //isolate top nibble
    g->available = 0;
    g->always_0 = 0; //same thing
    g->big = 0; //should leave zero according to manuals. No effect
    g->gran = 0; //so that our computed GDT limit is in bytes, not pages
    g->base_high = (base & 0xFF000000)>>24; //isolate top byte.

    // Ensure the TSS is initially zero'd.
    memset((char*)&tss_ent, 0, sizeof(tss_ent));

    extern int _end_stack;

    tss_ent.ss0  = 16;  // Set the kernel stack segment.
    tss_ent.esp0 = (uint32_t)&_end_stack; // Set the kernel stack pointer.
    tss_ent.cs   = 0x0b;
    tss_ent.ss = tss_ent.ds = tss_ent.es = tss_ent.fs = tss_ent.gs = 0x13;
    //note that CS is loaded from the IDT entry and should be the regular kernel code segment

    tss_flush(0x2b);
}






void PIC_sendEOI(unsigned char irq) {
	if(irq >= 8) {
		outb(PIC_2_COMMAND,PIC_EOI);
    }
	outb(PIC_1_COMMAND,PIC_EOI);
}

void IRQ_set_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;
 
    if(IRQline < 8) {
        port = PIC_1_DATA;
    } else {
        port = PIC_2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}
 
void IRQ_clear_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;
 
    if(IRQline < 8) {
        port = PIC_1_DATA;
    } else {
        port = PIC_2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}

void idt_flush(struct idt_ptr *idt){
    asm("lidt %0\n"
        :
        : "m"(*idt)
        :);
}


__attribute__((interrupt)) void divide_error_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}

__attribute__((interrupt)) void debug_exception_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void breakpoint_exception_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void overflow_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void bound_check_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void invalid_opcode_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void coprocessor_not_available_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}

__attribute__((interrupt)) void double_fault_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}


__attribute__((interrupt)) void coprocessor_segment_overrun_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}


__attribute__((interrupt)) void invalid_tss_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}


__attribute__((interrupt)) void segment_not_present_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}


__attribute__((interrupt)) void stack_exception_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
//    while(1);
}


//__attribute__((interrupt)) void general_protection_handler(struct interrupt_frame* frame)
void general_protection_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}
//void page_fault_handler(struct interrupt_frame* frame)
void page_fault_handler(struct process_context_with_error* ctx)
{
    asm("cli");
    while(1);
}


__attribute__((interrupt)) void coprocessor_error_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}

__attribute__((interrupt)) void stub_isr(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}

__attribute__((interrupt)) void pit_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}


__attribute__((interrupt)) void keyboard_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    outb(0x20,0x20);
}


__attribute__((interrupt)) void syscall_handler(struct interrupt_frame* frame)
{
    asm("cli");
    /* do something */
    while(1);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags /* | 0x60 */;
}

void init_idt() {
    int i;


    extern struct gdt_entry_bits gdt[];
    write_tss(&gdt[5]);

    idt_ptr.limit = sizeof(struct idt_entry) * 256 -1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset((char*)&idt_entries, 0, sizeof(struct idt_entry)*256);

    for(i = 0; i < 256; i++){
        idt_set_gate( i, (uint32_t)stub_isr, 0x08, 0x8E);
    }
    idt_set_gate(0, (uint32_t)divide_error_handler, 0x08, 0x8e);
    idt_set_gate(1, (uint32_t)debug_exception_handler, 0x08, 0x8e);
    idt_set_gate(2, (uint32_t)breakpoint_exception_handler, 0x08, 0x8e);
    idt_set_gate(3, (uint32_t)overflow_handler, 0x08, 0x8e);
    idt_set_gate(4, (uint32_t)overflow_handler, 0x08, 0x8e);
    idt_set_gate(5, (uint32_t)bound_check_handler, 0x08, 0x8e);
    idt_set_gate(6, (uint32_t)invalid_opcode_handler, 0x08, 0x8e);
    idt_set_gate(7, (uint32_t)coprocessor_not_available_handler, 0x08, 0x8e);
    idt_set_gate(8, (uint32_t)double_fault_handler, 0x08, 0x8e);
//    idt_set_gate(9, (uint32_t)coprocessor_segment_overrun_handler, 0x08, 0x8e);
    idt_set_gate(10, (uint32_t)invalid_tss_handler, 0x08, 0x8e);
    idt_set_gate(11, (uint32_t)segment_not_present_handler, 0x08, 0x8e);
    idt_set_gate(12, (uint32_t)stack_exception_handler, 0x08, 0x8e);

    idt_set_gate(13, (uint32_t)general_protection_handler, 0x08, 0x8e);
    idt_set_gate(14, (uint32_t)page_fault_handler, 0x08, 0x8e);
//    idt_set_gate(15, (uint32_t)coprocessor_error_handler, 0x08, 0x8e);

    idt_set_gate(0x21, (uint32_t)keyboard_handler,0x08, 0x8e);
    idt_set_gate(0x80, (uint32_t)syscall_handler,0x08, 0xee); // Set flags to EE, making DPL = 3 so it is accessible from userspace
    idt_set_gate(32,   (uint32_t)pit_handler, 0x08, 0x8e);
    idt_flush(&idt_ptr);
}

void remap_pic(void)
{
    /* ICW1 - begin initialization */
    outb(PIC_1_CTRL, 0x11);
    outb(PIC_2_CTRL, 0x11);

    /* ICW2 - remap offset address of idt_table */
    /*
    * In x86 protected mode, we have to remap the PICs beyond 0x20 because
    * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
    */
    outb(PIC_1_DATA, 0x20);
    outb(PIC_2_DATA, 0x28);

    /* ICW3 - setup cascading */
    outb(PIC_1_DATA, 0x00);
    outb(PIC_2_DATA, 0x00);

    /* ICW4 - environment info */
    outb(PIC_1_DATA, 0x01);
    outb(PIC_2_DATA, 0x01);
    /* mask interrupts */
    outb(0x21 , 0xff);
    outb(0xA1 , 0xff);
    /* Initialization finished */
    // outb(0x21, 0xfd); // Enable keyboard interrupts - DISABLED FOR TESTING
}
