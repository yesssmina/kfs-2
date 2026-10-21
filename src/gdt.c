#include "gdt.h"

/* Defined in gdt_flush.asm: loads GDTR and reloads segment registers */
extern void	gdt_flush(struct gdt_ptr *ptr);

/* Compile-time checks: layouts must match what the CPU reads */
_Static_assert(sizeof(struct gdt_entry) == 8, "gdt_entry must be 8 bytes");
_Static_assert(sizeof(struct gdt_ptr) == 6, "gdt_ptr must be 6 bytes");

static struct gdt_ptr	g_gdt_ptr;

/* Encode one descriptor directly in the table at GDT_ADDRESS */
static void	gdt_set_entry(int index, uint32_t base, uint32_t limit,
				uint8_t access, uint8_t flags)
{
	struct gdt_entry	*gdt = (struct gdt_entry *)GDT_ADDRESS;

	gdt[index].limit_low = limit & 0xFFFF;
	gdt[index].base_low = base & 0xFFFF;
	gdt[index].base_mid = (base >> 16) & 0xFF;
	gdt[index].access = access;
	gdt[index].granularity = ((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4);
	gdt[index].base_high = (base >> 24) & 0xFF;
}

/* Build the flat-model GDT at 0x800 and make the CPU use it */
void	gdt_init(void)
{
	gdt_set_entry(0, 0, 0, 0, 0);                                    /* null */
	gdt_set_entry(1, 0, 0xFFFFF, GDT_ACCESS_KCODE, GDT_FLAGS_32);    /* kernel code */
	gdt_set_entry(2, 0, 0xFFFFF, GDT_ACCESS_KDATA, GDT_FLAGS_32);    /* kernel data */
	gdt_set_entry(3, 0, 0xFFFFF, GDT_ACCESS_KDATA, GDT_FLAGS_32);    /* kernel stack */
	gdt_set_entry(4, 0, 0xFFFFF, GDT_ACCESS_UCODE, GDT_FLAGS_32);    /* user code */
	gdt_set_entry(5, 0, 0xFFFFF, GDT_ACCESS_UDATA, GDT_FLAGS_32);    /* user data */
	gdt_set_entry(6, 0, 0xFFFFF, GDT_ACCESS_UDATA, GDT_FLAGS_32);    /* user stack */

	g_gdt_ptr.limit = sizeof(struct gdt_entry) * GDT_ENTRIES - 1;
	g_gdt_ptr.base = GDT_ADDRESS;
	gdt_flush(&g_gdt_ptr);
}
