/* vga.c - VGA text mode driver: write characters directly to video memory */
#include "vga.h"
#include "io.h"
#include "string.h"

/* volatile: every write must really reach video memory, never be optimized out */
static volatile uint16_t *const	g_buffer = (volatile uint16_t *)VGA_MEMORY;
static size_t					g_row;
static size_t					g_col;
static uint8_t					g_color;

/* Saved state of each virtual screen; the active one lives in video memory */
typedef struct s_screen
{
	uint16_t	cells[VGA_WIDTH * VGA_HEIGHT];
	size_t		row;
	size_t		col;
	uint8_t		color;
}	t_screen;

static t_screen					g_screens[VGA_SCREENS];
static size_t					g_current;

/* Attribute byte: background in the high 4 bits, foreground in the low 4 bits */
static uint8_t	make_color(uint8_t fg, uint8_t bg)
{
	return ((uint8_t)((fg & 0x0F) | ((bg & 0x0F) << 4)));
}

/* One screen cell = 16 bits: attribute byte (high) + ASCII character (low) */
static uint16_t	make_entry(char c, uint8_t color)
{
	return ((uint16_t)((uint8_t)c | ((uint16_t)color << 8)));
}

/* Move the hardware (blinking) cursor to the current write position */
static void	update_cursor(void)
{
	uint16_t	pos;

	pos = (uint16_t)(g_row * VGA_WIDTH + g_col);
	outb(VGA_CRTC_INDEX, VGA_CURSOR_LOW);
	outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
	outb(VGA_CRTC_INDEX, VGA_CURSOR_HIGH);
	outb(VGA_CRTC_DATA, (uint8_t)(pos >> 8));
}

static void	clear_row(size_t row)
{
	size_t	col;

	col = 0;
	while (col < VGA_WIDTH)
	{
		g_buffer[row * VGA_WIDTH + col] = make_entry(' ', g_color);
		col++;
	}
}

/* Shift every row up by one (row 0 is lost) and clear the last row */
static void	scroll(void)
{
	memmove((void *)g_buffer, (const void *)(g_buffer + VGA_WIDTH),
		(VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));
	clear_row(VGA_HEIGHT - 1);
}

/* Go to the start of the next line, scrolling when the screen is full */
static void	new_line(void)
{
	g_col = 0;
	g_row++;
	if (g_row == VGA_HEIGHT)
	{
		scroll();
		g_row = VGA_HEIGHT - 1;
	}
}

void	vga_set_color(uint8_t fg, uint8_t bg)
{
	g_color = make_color(fg, bg);
}

/* Clear the screen and every saved virtual screen, start on screen 0 */
void	vga_init(void)
{
	size_t	row;
	size_t	i;

	g_row = 0;
	g_col = 0;
	g_color = make_color(VGA_LIGHT_GREY, VGA_BLACK);
	row = 0;
	while (row < VGA_HEIGHT)
	{
		clear_row(row);
		row++;
	}
	g_current = 0;
	i = 0;
	while (i < VGA_SCREENS)
	{
		memcpy(g_screens[i].cells, (const void *)g_buffer,
			sizeof(g_screens[i].cells));
		g_screens[i].row = 0;
		g_screens[i].col = 0;
		g_screens[i].color = g_color;
		i++;
	}
	update_cursor();
}

/* Save the active screen, then restore the requested one into video memory */
void	vga_switch_screen(size_t index)
{
	t_screen	*old;
	t_screen	*new;

	if (index >= VGA_SCREENS || index == g_current)
		return ;
	old = &g_screens[g_current];
	memcpy(old->cells, (const void *)g_buffer, sizeof(old->cells));
	old->row = g_row;
	old->col = g_col;
	old->color = g_color;
	new = &g_screens[index];
	memcpy((void *)g_buffer, new->cells, sizeof(new->cells));
	g_row = new->row;
	g_col = new->col;
	g_color = new->color;
	g_current = index;
	update_cursor();
}

size_t	vga_current_screen(void)
{
	return (g_current);
}

/* Move back one cell (to the previous line if needed) and erase it */
static void	backspace(void)
{
	if (g_col > 0)
		g_col--;
	else if (g_row > 0)
	{
		g_row--;
		g_col = VGA_WIDTH - 1;
	}
	g_buffer[g_row * VGA_WIDTH + g_col] = make_entry(' ', g_color);
}

void	vga_putchar(char c)
{
	if (c == '\n')
		new_line();
	else if (c == '\b')
		backspace();
	else
	{
		g_buffer[g_row * VGA_WIDTH + g_col] = make_entry(c, g_color);
		g_col++;
		if (g_col == VGA_WIDTH)
			new_line();
	}
	update_cursor();
}

void	vga_write(const char *str)
{
	while (*str)
	{
		vga_putchar(*str);
		str++;
	}
}
