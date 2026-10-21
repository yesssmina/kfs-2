/* io.h - x86 port I/O: talk to hardware devices (VGA cursor, keyboard...) */
#ifndef IO_H
# define IO_H

# include "types.h"

/* Write one byte to an I/O port (AT&T syntax: source first) */
static inline void	outb(uint16_t port, uint8_t value)
{
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read one byte from an I/O port */
static inline uint8_t	inb(uint16_t port)
{
	uint8_t	value;

	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return (value);
}

#endif
