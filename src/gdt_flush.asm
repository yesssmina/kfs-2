; gdt_flush(struct gdt_ptr *ptr)
; Load the new GDT, then reload every segment register so the CPU
; drops the descriptors it cached from GRUB's GDT.
bits 32
section .text
global gdt_flush

gdt_flush:
	mov eax, [esp + 4]      ; cdecl: first argument = pointer to GDTR operand
	lgdt [eax]              ; GDTR <- {limit, base}

	mov ax, 0x10            ; kernel data selector
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ax, 0x18            ; kernel stack selector
	mov ss, ax              ; base is 0 like before, so ESP stays valid

	jmp 0x08:.reload_cs     ; far jump: only way to reload CS (kernel code)
.reload_cs:
	ret
