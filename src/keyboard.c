/* keyboard.c - PS/2 keyboard driver: poll the controller, translate scancodes */
#include "keyboard.h"
#include "io.h"

/* Scancode set 1 -> ASCII, US QWERTY. Index = scancode, 0 = no character */
static const char	g_normal[] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0, ' '
};

/* Same keys with Shift held */
static const char	g_shift[] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	'*', 0, ' '
};

static int	g_shift_held;
static int	g_capslock;
static int	g_extended;

/* Busy-wait until the controller holds a keyboard byte, then return it */
static uint8_t	read_scancode(void)
{
	uint8_t	status;

	while (1)
	{
		status = inb(KBD_STATUS_PORT);
		if (status & KBD_OUTPUT_FULL)
		{
			if (!(status & KBD_AUX_DATA))
				return (inb(KBD_DATA_PORT));
			inb(KBD_DATA_PORT);
		}
	}
}

/* Caps Lock only affects letters: swap their case */
static char	apply_capslock(char c)
{
	if (c >= 'a' && c <= 'z')
		return ((char)(c - 'a' + 'A'));
	if (c >= 'A' && c <= 'Z')
		return ((char)(c - 'A' + 'a'));
	return (c);
}

/* Update modifier state; return 1 if the scancode was a modifier */
static int	handle_modifier(uint8_t code)
{
	uint8_t	key;

	key = code & ~SC_RELEASE;
	if (key == SC_LSHIFT || key == SC_RSHIFT)
	{
		g_shift_held = !(code & SC_RELEASE);
		return (1);
	}
	if (code == SC_CAPSLOCK)
	{
		g_capslock = !g_capslock;
		return (1);
	}
	return (0);
}

int	keyboard_getkey(void)
{
	uint8_t	code;
	char	c;

	while (1)
	{
		code = read_scancode();
		if (code == SC_EXTENDED)
		{
			g_extended = 1;
			continue ;
		}
		if (g_extended)
		{
			g_extended = 0;
			continue ;
		}
		if (handle_modifier(code) || (code & SC_RELEASE))
			continue ;
		if (code >= SC_F1 && code <= SC_F4)
			return (KEY_F1 + (code - SC_F1));
		if (code >= sizeof(g_normal))
			continue ;
		c = g_shift_held ? g_shift[code] : g_normal[code];
		if (c == 0)
			continue ;
		if (g_capslock)
			c = apply_capslock(c);
		return ((unsigned char)c);
	}
}
