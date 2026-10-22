#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "printk.h"
#include "string.h"
#include "io.h"
#include "stack.h"
#include "gdt.h"

#define SHELL_PROMPT		"> "
#define SHELL_LINE_MAX		64	/* max characters per command line */

#define KBC_STATUS			0x64	/* 8042 status (read) / command (write) port */
#define KBC_INPUT_FULL		0x02	/* status bit 1: controller not ready yet */
#define KBC_CMD_RESET		0xFE	/* pulse the CPU reset line */

/* One shell command: its name, a short description, the function to run */
struct shell_command {
	const char	*name;
	const char	*help;
	void		(*run)(void);
};

static void	cmd_help(void);
static void	cmd_reboot(void);
static void	cmd_halt(void);

/* Command table: adding a command = adding one line here */
static const struct shell_command	g_commands[] = {
	{"help", "list the commands", cmd_help},
	{"stack", "print the kernel stack", stack_print},
	{"gdt", "print the loaded GDT", gdt_print},
	{"reboot", "restart the machine", cmd_reboot},
	{"halt", "stop the CPU", cmd_halt},
};

#define SHELL_NB_COMMANDS	(sizeof(g_commands) / sizeof(g_commands[0]))

/* One line buffer per virtual screen (zeroed: .bss) */
static char		g_line[VGA_SCREENS][SHELL_LINE_MAX + 1];
static size_t	g_len[VGA_SCREENS];

static void	cmd_help(void)
{
	size_t	i;

	printk("Commands:\n");
	i = 0;
	while (i < SHELL_NB_COMMANDS)
	{
		printk("  %s - %s\n", g_commands[i].name, g_commands[i].help);
		i++;
	}
}

/* Disable interrupts and halt; loop in case an NMI wakes the CPU up */
static void	cmd_halt(void)
{
	printk("System halted.\n");
	while (1)
		__asm__ volatile ("cli; hlt");
}

/* Ask the keyboard controller to reset the CPU */
static void	cmd_reboot(void)
{
	printk("Rebooting...\n");
	while (inb(KBC_STATUS) & KBC_INPUT_FULL)
	{
		/* wait until the controller can accept a command */
	}
	outb(KBC_STATUS, KBC_CMD_RESET);
	cmd_halt();	/* only reached if the reset failed */
}

void	shell_prompt(void)
{
	vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
	vga_write(SHELL_PROMPT);
	vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* Find the typed line in the command table and run it */
static void	shell_execute(const char *line)
{
	size_t	i;

	if (line[0] == '\0')
		return ;
	i = 0;
	while (i < SHELL_NB_COMMANDS)
	{
		if (strcmp(line, g_commands[i].name) == 0)
		{
			g_commands[i].run();
			return ;
		}
		i++;
	}
	printk("unknown command: %s (type 'help')\n", line);
}

/* Feed one character to the line buffer of the current screen */
static void	shell_handle_char(char c)
{
	size_t	screen;

	screen = vga_current_screen();
	if (c == '\b')
	{
		if (g_len[screen] > 0)
		{
			vga_putchar('\b');
			g_len[screen]--;
		}
	}
	else if (c == '\n')
	{
		vga_putchar('\n');
		g_line[screen][g_len[screen]] = '\0';
		g_len[screen] = 0;
		shell_execute(g_line[screen]);
		shell_prompt();
	}
	else if (c >= ' ' && c <= '~' && g_len[screen] < SHELL_LINE_MAX)
	{
		vga_putchar(c);
		g_line[screen][g_len[screen]] = c;
		g_len[screen]++;
	}
}

void	shell_run(void)
{
	int	key;

	shell_prompt();
	while (1)
	{
		key = keyboard_getkey();
		if (key >= KEY_F1 && key < KEY_F1 + VGA_SCREENS)
			vga_switch_screen((size_t)(key - KEY_F1));
		else
			shell_handle_char((char)key);
	}
}
