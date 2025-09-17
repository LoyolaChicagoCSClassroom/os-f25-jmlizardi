#pragma once
#include <stdint.h>

// Read a byte from I/O port
#ifndef IO_H
#define IO_H

// Write a byte to the specified port
static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from the specified port
/*static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}
*/
// Read a word (2 bytes) from the specified port
static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

#endif // IO_H

