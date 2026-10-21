; boot.asm - kernel entry point
; GRUB finds the multiboot header, loads the kernel at 1 MiB in 32-bit
; protected mode, then jumps to _start. We set up a stack and call C.

; ---- Multiboot header constants ----
MBALIGN  equ 1 << 0              ; flag bit 0: align loaded modules on 4 KiB pages
MEMINFO  equ 1 << 1              ; flag bit 1: ask GRUB for a memory map
MBFLAGS  equ MBALIGN | MEMINFO   ; = 3
MAGIC    equ 0x1BADB002          ; multiboot 1 signature
CHECKSUM equ -(MAGIC + MBFLAGS)  ; magic + flags + checksum must equal 0

; ---- Multiboot header ----
; Own section so the linker script can put it first (must be in the first 8 KiB)
section .multiboot
align 4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

; ---- Kernel stack ----
; .bss = reserved, uninitialized memory: takes no space in the binary
section .bss
align 16                         ; i386 System V ABI: 16-byte aligned stack
stack_bottom:
    resb 16384                   ; 16 KiB
stack_top:                       ; x86 stack grows downward: start from the top

; ---- Entry point ----
section .text
global _start                    ; visible to the linker (ENTRY in linker.ld)
extern kernel_main
extern gdt_init               ; defined in kernel.c

_start:
    mov esp, stack_top           ; ESP is undefined after GRUB: set our stack
    xor ebp, ebp            ; end of the frame chain for stack traces
    call gdt_init            ; install our GDT before anything else
    call kernel_main             ; enter C code

    ; kernel_main should never return; if it does, halt the CPU forever
    cli                          ; disable interrupts
.hang:
    hlt                          ; sleep until next interrupt
    jmp .hang                    ; if woken up anyway, sleep again
