/* vga.h - VGA text mode driver interface */
#ifndef VGA_H
# define VGA_H

# include "types.h"

# define VGA_WIDTH		80
# define VGA_HEIGHT		25
# define VGA_MEMORY		0xB8000
# define VGA_SCREENS		4		/* virtual screens, switched with F1..F4 */

/* CRT controller ports: select a register via INDEX, then access it via DATA */
# define VGA_CRTC_INDEX	0x3D4
# define VGA_CRTC_DATA	0x3D5
# define VGA_CURSOR_HIGH	0x0E
# define VGA_CURSOR_LOW	0x0F

enum e_vga_color
{
	VGA_BLACK = 0,
	VGA_BLUE = 1,
	VGA_GREEN = 2,
	VGA_CYAN = 3,
	VGA_RED = 4,
	VGA_MAGENTA = 5,
	VGA_BROWN = 6,
	VGA_LIGHT_GREY = 7,
	VGA_DARK_GREY = 8,
	VGA_LIGHT_BLUE = 9,
	VGA_LIGHT_GREEN = 10,
	VGA_LIGHT_CYAN = 11,
	VGA_LIGHT_RED = 12,
	VGA_LIGHT_MAGENTA = 13,
	VGA_YELLOW = 14,
	VGA_WHITE = 15
};

void	vga_init(void);
void	vga_set_color(uint8_t fg, uint8_t bg);
void	vga_putchar(char c);
void	vga_write(const char *str);
void	vga_switch_screen(size_t index);
size_t	vga_current_screen(void);

#endif
