#ifndef GDT_H
#define GDT_H

#include "types.h"

/* The subject requires the GDT to live at this physical address */
#define GDT_ADDRESS        0x00000800
#define GDT_ENTRIES        7

/* Segment selectors: index * 8 (TI = 0, RPL = 0) */
#define GDT_NULL           0x00
#define GDT_KERNEL_CODE    0x08
#define GDT_KERNEL_DATA    0x10
#define GDT_KERNEL_STACK   0x18
#define GDT_USER_CODE      0x20
#define GDT_USER_DATA      0x28
#define GDT_USER_STACK     0x30

/* Access bytes: P | DPL | S | E | DC | RW | A */
#define GDT_ACCESS_KCODE   0x9A    /* present, ring 0, code, readable */
#define GDT_ACCESS_KDATA   0x92    /* present, ring 0, data, writable */
#define GDT_ACCESS_UCODE   0xFA    /* present, ring 3, code, readable */
#define GDT_ACCESS_UDATA   0xF2    /* present, ring 3, data, writable */

/* Flags nibble: G = 1 (4 KiB granularity), DB = 1 (32-bit) */
#define GDT_FLAGS_32       0xC

/* One 8-byte segment descriptor, fields split as the CPU expects */
struct gdt_entry {
	uint16_t	limit_low;     /* limit 15..0 */
	uint16_t	base_low;      /* base 15..0 */
	uint8_t		base_mid;      /* base 23..16 */
	uint8_t		access;        /* access byte */
	uint8_t		granularity;   /* flags (high nibble) | limit 19..16 (low nibble) */
	uint8_t		base_high;     /* base 31..24 */
} __attribute__((packed));

/* 6-byte operand of the lgdt instruction */
struct gdt_ptr {
	uint16_t	limit;         /* table size - 1 */
	uint32_t	base;          /* linear address of the table */
} __attribute__((packed));

void	gdt_init(void);
void	gdt_print(void);

#endif
