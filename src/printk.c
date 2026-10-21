/* printk.c - minimal printf for the kernel, writing to the VGA screen */
#include "printk.h"
#include "stdarg.h"
#include "types.h"
#include "vga.h"

/* Parsed conversion: %[0][width]conv */
typedef struct s_spec
{
	char	pad;
	int		width;
	char	conv;
}	t_spec;

static int	put_char(char c)
{
	vga_putchar(c);
	return (1);
}

static int	put_str(const char *s)
{
	int	n;

	if (!s)
		s = "(null)";
	n = 0;
	while (*s)
		n += put_char(*s++);
	return (n);
}

static int	put_repeat(char c, int count)
{
	int	n;

	n = 0;
	while (count-- > 0)
		n += put_char(c);
	return (n);
}

/* String or char with width: right-aligned, padded with spaces */
static int	put_padded_str(const char *s, const t_spec *spec)
{
	int	len;

	if (!s)
		s = "(null)";
	len = 0;
	while (s[len])
		len++;
	return (put_repeat(' ', spec->width - len) + put_str(s));
}

/* Print |value| in base, with sign and padding: zeros go after '-', spaces before */
static int	put_number(uint32_t value, int negative, unsigned int base,
	const t_spec *spec)
{
	const char	*digits;
	char		buf[32];
	int			len;
	int			n;

	digits = (spec->conv == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
	len = 0;
	do
	{
		buf[len++] = digits[value % base];
		value /= base;
	} while (value);
	n = 0;
	if (spec->pad == ' ')
		n += put_repeat(' ', spec->width - len - negative);
	if (negative)
		n += put_char('-');
	if (spec->pad == '0')
		n += put_repeat('0', spec->width - len - negative);
	while (len)
		n += put_char(buf[--len]);
	return (n);
}

/* Signed decimal; INT_MIN is handled without overflow */
static int	put_signed(int32_t value, const t_spec *spec)
{
	if (value < 0)
		return (put_number((uint32_t)(-(value + 1)) + 1, 1, 10, spec));
	return (put_number((uint32_t)value, 0, 10, spec));
}

static int	put_conversion(const t_spec *spec, va_list *ap)
{
	t_spec	ptr;
	char	c[2];

	if (spec->conv == 'c')
	{
		c[0] = (char)va_arg(*ap, int);
		c[1] = '\0';
		return (put_padded_str(c, spec));
	}
	if (spec->conv == 's')
		return (put_padded_str(va_arg(*ap, const char *), spec));
	if (spec->conv == 'd' || spec->conv == 'i')
		return (put_signed(va_arg(*ap, int32_t), spec));
	if (spec->conv == 'u')
		return (put_number(va_arg(*ap, uint32_t), 0, 10, spec));
	if (spec->conv == 'x' || spec->conv == 'X')
		return (put_number(va_arg(*ap, uint32_t), 0, 16, spec));
	if (spec->conv == 'p')
	{
		ptr.pad = '0';
		ptr.width = 8;
		ptr.conv = 'x';
		return (put_str("0x")
			+ put_number((uint32_t)va_arg(*ap, void *), 0, 16, &ptr));
	}
	if (spec->conv == '%')
		return (put_char('%'));
	return (put_char('%') + put_char(spec->conv));
}

/* Read the flag and width after '%'; return a pointer to the conversion char */
static const char	*parse_spec(const char *fmt, t_spec *spec)
{
	spec->pad = ' ';
	spec->width = 0;
	if (*fmt == '0')
	{
		spec->pad = '0';
		fmt++;
	}
	while (*fmt >= '0' && *fmt <= '9')
		spec->width = spec->width * 10 + (*fmt++ - '0');
	spec->conv = *fmt;
	return (fmt);
}

int	printk(const char *fmt, ...)
{
	va_list	ap;
	t_spec	spec;
	int		n;

	va_start(ap, fmt);
	n = 0;
	while (*fmt)
	{
		if (*fmt == '%' && fmt[1])
		{
			fmt = parse_spec(fmt + 1, &spec);
			if (!*fmt)
				break ;
			n += put_conversion(&spec, &ap);
		}
		else
			n += put_char(*fmt);
		fmt++;
	}
	va_end(ap);
	return (n);
}
