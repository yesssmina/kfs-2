#include "stack.h"
#include "printk.h"
#include "types.h"

#define STACK_MAX_FRAMES	16	/* hard limit when walking the chain */
#define STACK_DUMP_WORDS	32	/* raw dump: 8 lines of 4 words */

/* Stack bounds, exported as globals by boot.asm */
extern uint8_t	stack_bottom[];
extern uint8_t	stack_top[];

static inline __attribute__((always_inline)) uint32_t	read_esp(void)
{
	uint32_t	esp;

	__asm__ volatile ("mov %%esp, %0" : "=r"(esp));
	return (esp);
}

static inline __attribute__((always_inline)) uint32_t	read_ebp(void)
{
	uint32_t	ebp;

	__asm__ volatile ("mov %%ebp, %0" : "=r"(ebp));
	return (ebp);
}

/* True if [addr, addr + len) lies inside the kernel stack and addr is aligned */
static int	in_stack(uint32_t addr, uint32_t len)
{
	return ((addr & 3) == 0
		&& addr >= (uint32_t)stack_bottom
		&& addr + len <= (uint32_t)stack_top);
}

/* Walk the saved-EBP chain: [ebp] = caller's EBP, [ebp + 4] = return address */
static void	stack_print_trace(uint32_t ebp)
{
	uint32_t	*frame;
	int			depth;

	printk("Call trace (most recent first):\n");
	depth = 0;
	while (ebp != 0 && depth < STACK_MAX_FRAMES)
	{
		if (!in_stack(ebp, 8))
		{
			printk("  <invalid frame pointer 0x%08x>\n", ebp);
			return ;
		}
		frame = (uint32_t *)ebp;
		printk("  #%d ebp 0x%08x  ret 0x%08x", depth, ebp, frame[1]);
		if (in_stack(ebp, 16))
			printk("  [ebp+8] 0x%08x [ebp+12] 0x%08x", frame[2], frame[3]);
		printk("\n");
		/* Caller frames live at higher addresses: anything else is corruption */
		if (frame[0] != 0 && frame[0] <= ebp)
		{
			printk("  <broken chain: next ebp 0x%08x>\n", frame[0]);
			return ;
		}
		ebp = frame[0];
		depth++;
	}
	if (ebp == 0)
		printk("  -- end of chain (ebp = 0) --\n");
}

/* Hexdump the live part of the stack, starting at ESP, 4 words per line */
static void	stack_print_dump(uint32_t esp)
{
	uint32_t	addr;
	int			i;

	printk("Raw stack from esp (low -> high addresses):\n");
	addr = esp & ~3u;
	i = 0;
	while (i < STACK_DUMP_WORDS && addr < (uint32_t)stack_top)
	{
		if (i % 4 == 0)
			printk("  0x%08x:", addr);
		printk(" %08x", *(uint32_t *)addr);
		if (i % 4 == 3)
			printk("\n");
		addr += 4;
		i++;
	}
	if (i % 4 != 0)
		printk("\n");
}

void	stack_print(void)
{
	uint32_t	esp;
	uint32_t	ebp;
	uint32_t	bottom;
	uint32_t	top;

	esp = read_esp();
	ebp = read_ebp();
	bottom = (uint32_t)stack_bottom;
	top = (uint32_t)stack_top;
	printk("--- Kernel stack ---\n");
	printk("range 0x%08x - 0x%08x (%u bytes)\n", bottom, top, top - bottom);
	printk("esp 0x%08x  ebp 0x%08x  used %u bytes\n", esp, ebp, top - esp);
	stack_print_trace(ebp);
	stack_print_dump(esp);
}
