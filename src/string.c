/* string.c - kernel string and memory helpers (our own, no libc) */
#include "string.h"

size_t	strlen(const char *s)
{
	size_t	len;

	len = 0;
	while (s[len])
		len++;
	return (len);
}

/* Compare as unsigned char, like the standard: result sign gives the order */
int	strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s1 == *s2)
	{
		s1++;
		s2++;
	}
	return ((unsigned char)*s1 - (unsigned char)*s2);
}

int	strncmp(const char *s1, const char *s2, size_t n)
{
	while (n && *s1 && *s1 == *s2)
	{
		s1++;
		s2++;
		n--;
	}
	if (n == 0)
		return (0);
	return ((unsigned char)*s1 - (unsigned char)*s2);
}

/* gcc may emit calls to memset/memcpy/memmove/memcmp even in freestanding
   mode, so these four must exist with their standard names */
void	*memset(void *s, int c, size_t n)
{
	uint8_t	*p;

	p = (uint8_t *)s;
	while (n--)
		*p++ = (uint8_t)c;
	return (s);
}

/* Memory areas must not overlap: use memmove otherwise */
void	*memcpy(void *dst, const void *src, size_t n)
{
	uint8_t			*d;
	const uint8_t	*s;

	d = (uint8_t *)dst;
	s = (const uint8_t *)src;
	while (n--)
		*d++ = *s++;
	return (dst);
}

/* Overlap-safe copy: copy backward when dst is after src */
void	*memmove(void *dst, const void *src, size_t n)
{
	uint8_t			*d;
	const uint8_t	*s;

	d = (uint8_t *)dst;
	s = (const uint8_t *)src;
	if (d < s)
	{
		while (n--)
			*d++ = *s++;
	}
	else if (d > s)
	{
		while (n--)
			d[n] = s[n];
	}
	return (dst);
}

int	memcmp(const void *s1, const void *s2, size_t n)
{
	const uint8_t	*a;
	const uint8_t	*b;

	a = (const uint8_t *)s1;
	b = (const uint8_t *)s2;
	while (n--)
	{
		if (*a != *b)
			return (*a - *b);
		a++;
		b++;
	}
	return (0);
}
