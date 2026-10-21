/* linker.ld - our own linker script (the host's default one is forbidden) */

/* GRUB jumps to this symbol (written in the ELF header) */
ENTRY(_start)

SECTIONS
{
	/* GRUB loads the kernel at 1 MiB: below is BIOS data, VGA memory, ROM */
	. = 1M;

	/* Multiboot header first: it must be within the first 8 KiB */
	/* 4 KiB alignment = x86 page size, ready for paging later in KFS */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/* Read-only data (string literals, constants) */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata*)
	}

	/* Initialized global variables */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Uninitialized global variables and our 16 KiB stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* Host-specific sections we do not need */
	/DISCARD/ :
	{
		*(.eh_frame)
		*(.comment)
		*(.note*)
	}
}
