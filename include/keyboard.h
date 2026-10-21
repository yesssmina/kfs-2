/* keyboard.h - PS/2 keyboard driver (polling, US QWERTY layout) */
#ifndef KEYBOARD_H
# define KEYBOARD_H

# include "types.h"

/* PS/2 controller ports */
# define KBD_DATA_PORT		0x60	/* read: scancode sent by the keyboard */
# define KBD_STATUS_PORT	0x64	/* read: controller status */

/* Status register bits */
# define KBD_OUTPUT_FULL	0x01	/* a byte is waiting in the data port */
# define KBD_AUX_DATA		0x20	/* that byte comes from the mouse, not the keyboard */

/* Scancodes (set 1) */
# define SC_RELEASE		0x80	/* bit set on key release */
# define SC_EXTENDED	0xE0	/* prefix of extended keys (arrows...) */
# define SC_LSHIFT		0x2A
# define SC_RSHIFT		0x36
# define SC_CAPSLOCK	0x3A
# define SC_F1			0x3B	/* F1..F4 are consecutive: 0x3B..0x3E */
# define SC_F4			0x3E

/* Special keys returned by keyboard_getkey, outside the ASCII range */
# define KEY_F1			0x100

/* Wait for a key press: return its ASCII character ('\n' Enter, '\b' Backspace)
   or KEY_F1 + n for function key Fn+1 */
int		keyboard_getkey(void);

#endif
