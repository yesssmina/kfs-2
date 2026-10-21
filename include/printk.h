/* printk.h - formatted output for the kernel */
#ifndef PRINTK_H
# define PRINTK_H

/* Supported: %c %s %d %i %u %x %X %p %%, optional '0' flag and width (%08x) */
int	printk(const char *fmt, ...);

#endif
