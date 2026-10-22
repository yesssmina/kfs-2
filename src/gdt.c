#include "gdt.h"
#include "printk.h"

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

/* Human-readable names, indexed by descriptor number */
static const char	*g_gdt_names[GDT_ENTRIES] = {
	"null", "kernel code", "kernel data", "kernel stack",
	"user code", "user data", "user stack"
};

/* Ask the CPU which GDT is loaded (sgdt) and decode every descriptor */
void	gdt_print(void)
{
	struct gdt_ptr		gdtr;
	struct gdt_entry	*e;
	uint16_t			cs;
	uint16_t			ds;
	uint16_t			ss;
	uint32_t			base;
	uint32_t			limit;
	uint32_t			count;
	uint32_t			i;
	const char			*type;

	__asm__ volatile ("sgdt %0" : "=m"(gdtr));
	__asm__ volatile ("mov %%cs, %0" : "=r"(cs));
	__asm__ volatile ("mov %%ds, %0" : "=r"(ds));
	__asm__ volatile ("mov %%ss, %0" : "=r"(ss));
	count = (gdtr.limit + 1) / sizeof(struct gdt_entry);
	printk("--- GDT ---\n");
	printk("GDTR base 0x%08x  limit 0x%04x (%u entries)\n",
		gdtr.base, (uint32_t)gdtr.limit, count);
	printk("cs 0x%04x  ds 0x%04x  ss 0x%04x\n\n",
		(uint32_t)cs, (uint32_t)ds, (uint32_t)ss);
	printk("  sel  base       limit       acc fl dpl type  name\n");
	i = 0;
	while (i < count)
	{
		e = (struct gdt_entry *)gdtr.base + i;
		base = e->base_low | ((uint32_t)e->base_mid << 16)
			| ((uint32_t)e->base_high << 24);
		limit = e->limit_low | ((uint32_t)(e->granularity & 0x0F) << 16);
		if (e->granularity & 0x80)
			limit = (limit << 12) | 0xFFF;
		if (!(e->access & 0x80))
			type = "null";
		else if (e->access & 0x08)
			type = "code";
		else
			type = "data";
		printk("  0x%02x 0x%08x 0x%08x  %02x  %x  %u   %s  %s\n",
			i * 8, base, limit, (uint32_t)e->access,
			(uint32_t)(e->granularity >> 4), (uint32_t)((e->access >> 5) & 3),
			type, i < GDT_ENTRIES ? g_gdt_names[i] : "?");
		i++;
	}
}
