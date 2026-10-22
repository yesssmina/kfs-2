/* kernel.c - main function of the kernel, called by boot.asm */
#include "vga.h"
#include "keyboard.h"
#include "printk.h"
#include "stack.h"
#include "gdt.h"
#include "string.h"

#define FULL_BLOCK	'\xDB'	/* code page 437 character: a filled cell */
#define PROMPT		"> "

/* Print one self-test line: green OK or red KO */
static void	check(const char *name, int ok)
{
	vga_set_color(ok ? VGA_LIGHT_GREEN : VGA_LIGHT_RED, VGA_BLACK);
	vga_write(ok ? "[OK] " : "[KO] ");
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	vga_write(name);
	vga_putchar('\n');
}

/* Boot-time sanity checks of the kernel helpers */
static void	selftest(void)
{
	char	buf[8];

	check("strlen", strlen("") == 0 && strlen("42") == 2);
	check("strcmp", strcmp("42", "42") == 0 && strcmp("a", "b") < 0
		&& strcmp("b", "a") > 0);
	check("strncmp", strncmp("kfs-1", "kfs-2", 4) == 0
		&& strncmp("kfs-1", "kfs-2", 5) < 0);
	memset(buf, 'x', 4);
	check("memset", memcmp(buf, "xxxx", 4) == 0);
	memcpy(buf, "abcd", 5);
	check("memcpy", strcmp(buf, "abcd") == 0);
	memmove(buf + 1, buf, 4);
	check("memmove", memcmp(buf, "aabcd", 5) == 0);
	check("memcmp", memcmp("ab", "ac", 2) < 0 && memcmp("ab", "ab", 2) == 0);
}

/* Bonus colors: one filled block per foreground color */
static void	color_demo(void)
{
	uint8_t	color;

	vga_write("colors: ");
	color = 0;
	while (color < 16)
	{
		vga_set_color(color, VGA_BLACK);
		vga_putchar(FULL_BLOCK);
		vga_putchar(FULL_BLOCK);
		color++;
	}
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	vga_putchar('\n');
}

static void	print_prompt(void)
{
	vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	vga_write(PROMPT);
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* Bonus screens: give screens 2..4 a header and a prompt, then back to 1 */
static void	setup_screens(void)
{
	size_t	i;

	i = 1;
	while (i < VGA_SCREENS)
	{
		vga_switch_screen(i);
		vga_set_color(VGA_YELLOW, VGA_BLACK);
		printk("Screen %u/%u", (uint32_t)(i + 1), (uint32_t)VGA_SCREENS);
		vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
		printk(" - F1..F%u to switch\n\n", (uint32_t)VGA_SCREENS);
		if (i == 1)
			stack_print();
		else if (i == 2)
			gdt_print();
		print_prompt();
		i++;
	}
	vga_switch_screen(0);
}

/* Echo one typed character; Backspace never erases the prompt */
static void	handle_char(char c, size_t *len)
{
	if (c == '\b')
	{
		if (*len > 0)
		{
			vga_putchar('\b');
			(*len)--;
		}
	}
	else if (c == '\n')
	{
		vga_putchar('\n');
		print_prompt();
		*len = 0;
	}
	else
	{
		vga_putchar(c);
		(*len)++;
	}
}

/* Bonus keyboard: F1..F4 switch screens, other keys are echoed */
static void	echo_loop(void)
{
	size_t	len[VGA_SCREENS];
	int		key;

	memset(len, 0, sizeof(len));
	print_prompt();
	while (1)
	{
		key = keyboard_getkey();
		if (key >= KEY_F1 && key < KEY_F1 + VGA_SCREENS)
			vga_switch_screen((size_t)(key - KEY_F1));
		else
			handle_char((char)key, &len[vga_current_screen()]);
	}
}

void	kernel_main(void)
{
	vga_init();
	setup_screens();
	vga_set_color(VGA_WHITE, VGA_BLACK);
	vga_write("42\n\n");
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
	selftest();
	vga_putchar('\n');
	color_demo();
	printk("printk: %s | %d | %u | %x | %p | %c | %08x | %%\n",
		"str", -42, 42, 0xbeef, (void *)VGA_MEMORY, 'K', 42);
	vga_write("\nType on the keyboard (Enter: new line, Backspace: erase,"
		" F1..F4: screens)\n");
	echo_loop();
}
