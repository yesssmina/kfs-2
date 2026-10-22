# KFS-2 — GDT & Stack

> Second step of the **Kernel From Scratch** series (i386, C + NASM), built on KFS-1.
> The kernel installs its **own Global Descriptor Table at `0x00000800`**, prints its **kernel stack** in a human-readable way, and offers a small **debug shell**.

```sh
make        # build kernel.bin and the bootable image kfs.iso
make run    # boot it in QEMU, then F2: kernel stack, F3: GDT, type 'help'
```

## Contents

1. [Features](#features)
2. [Build and run](#build-and-run)
3. [Project layout](#project-layout)
4. [How it boots](#how-it-boots)
5. [The GDT](#the-gdt)
6. [The kernel stack tool](#the-kernel-stack-tool)
7. [Screens and shell](#screens-and-shell)
8. [Defense guide](#defense-guide)
9. [Debugging with GDB](#debugging-with-gdb)
10. [Design notes](#design-notes)

## Features

| | Feature | Part |
|:-:|---|:-:|
| ✅ | GDT with kernel code / data / stack and user code / data / stack | Mandatory |
| ✅ | GDT located at `0x00000800` | Mandatory |
| ✅ | GDT declared to the CPU (`lgdt`) and every segment register reloaded | Mandatory |
| ✅ | Human-readable kernel stack printer (bounds, call trace, raw dump) | Mandatory |
| ✅ | Own linker script, freestanding flags, i386, image under 10 MB | Mandatory |
| ✅ | Debug shell: `help`, `stack`, `gdt`, `reboot`, `halt` | Bonus |
| ✅ | From KFS-1: VGA driver, colors, scroll, cursor, `printk`, keyboard, 4 screens | Base |

## Build and run

| Command | Effect |
|---|---|
| `./install_deps.sh` | Install and check the toolchain |
| `make` | Build `kernel.bin` and `kfs.iso` |
| `make run` | Boot `kfs.iso` in QEMU (KVM when available) |
| `make check` | Check the Multiboot header and the size limit |
| `make V=1 re` | Full rebuild showing every command and flag |
| `make clean` / `make fclean` | Remove objects / objects and binaries |

## Project layout

```
.
├── Makefile           build rules for ASM and C, link, image
├── linker.ld          our own linker script
├── grub.cfg           GRUB menu entry
├── kfs.iso            bootable image (turned in)
├── include/           headers (gdt.h, stack.h, shell.h, vga.h, ...)
└── src/
    ├── boot.asm       Multiboot header, stack, _start
    ├── gdt.c          builds the GDT at 0x800, decodes it (gdt_print)
    ├── gdt_flush.asm  lgdt + reload of every segment register
    ├── stack.c        kernel stack printer (stack_print)
    ├── shell.c        debug shell: line buffer and command table
    ├── kernel.c       kernel_main: 42, self-tests, screens, shell
    ├── vga.c          VGA driver: colors, scroll, cursor, screens
    ├── keyboard.c     PS/2 keyboard driver (polling)
    ├── printk.c       formatted output
    └── string.c       strlen, strcmp, strncmp, mem*
```

## How it boots

```
 GRUB ──► _start (protected mode, GRUB's temporary GDT)
            │  mov esp, stack_top        kernel stack (16 KiB in .bss)
            │  xor ebp, ebp              end marker of the frame chain
            │  call gdt_init
            │      ├─ writes 7 descriptors at 0x800
            │      └─ gdt_flush: lgdt
            │                    DS/ES/FS/GS = 0x10, SS = 0x18
            │                    jmp 0x08:...  (reloads CS)
            │  call kernel_main
            ▼
 kernel_main ──► F2: stack_print()  F3: gdt_print()  F1: "42" + shell
```

| Address | Content |
|---|---|
| `0x00000000` | BIOS interrupt vector table |
| `0x00000400` | BIOS data area |
| `0x00000800` – `0x00000837` | **Our GDT** (7 × 8 bytes) |
| `0x000B8000` | VGA text buffer |
| `0x00100000` | Kernel image, entry `0x100010`; the stack lives in its `.bss` |

## The GDT

Flat model: every segment has base `0` and limit `0xFFFFF` with 4 KiB granularity (4 GiB). Segments only differ by **type** (code / data) and **privilege** (ring 0 / ring 3). A stack segment is a writable data segment loaded into `SS`.

| Selector | Segment | Access | Flags | DPL | Descriptor |
|---|---|:-:|:-:|:-:|---|
| `0x00` | null (required by the CPU) | `00` | `0` | – | `0x0000000000000000` |
| `0x08` | kernel code | `9A` | `C` | 0 | `0x00CF9A000000FFFF` |
| `0x10` | kernel data | `92` | `C` | 0 | `0x00CF92000000FFFF` |
| `0x18` | kernel stack | `92` | `C` | 0 | `0x00CF92000000FFFF` |
| `0x20` | user code | `FA` | `C` | 3 | `0x00CFFA000000FFFF` |
| `0x28` | user data | `F2` | `C` | 3 | `0x00CFF2000000FFFF` |
| `0x30` | user stack | `F2` | `C` | 3 | `0x00CFF2000000FFFF` |

- **Access byte**: `P | DPL(2) | S | E | DC | RW | A`. Flags `C` = 4 KiB granularity + 32-bit.
- **GDTR**: base `0x800`, limit `7 × 8 − 1 = 0x37`.
- Once loaded, the CPU sets the *accessed* bit: `92` becomes `93`, `9A` becomes `9B`. User segments, never loaded, stay `F2` / `FA`.

## The kernel stack tool

`stack_print()` (`src/stack.c`) prints:

1. the stack bounds (`stack_bottom`, `stack_top`), `ESP`, `EBP` and the bytes in use;
2. the **call trace**, by walking the saved-EBP chain;
3. a raw dump of the stack from `ESP`, 4 words per line.

```
higher addresses
  [ebp+8]   first argument (cdecl)
  [ebp+4]   return address        <- printed for each frame
  [ebp]     caller's EBP          <- next frame
  [ebp-4]   local variables
lower addresses  <- ESP
```

The walk stops at `EBP = 0` (set by `_start`). Safety checks: each EBP must be inside the stack, 4-byte aligned and strictly higher than the previous one; at most 16 frames.

## Screens and shell

| Key | Screen |
|:-:|---|
| F1 | Boot output (`42`, self-tests, colors, `printk`) and shell |
| F2 | Kernel stack, printed at boot, then a shell |
| F3 | Loaded GDT, printed at boot, then a shell |
| F4 | Shell |

Each screen has its own line buffer. Commands:

| Command | Effect |
|---|---|
| `help` | List the commands |
| `stack` | Print the kernel stack |
| `gdt` | Print the GDT the CPU is using (`sgdt`) |
| `reboot` | Reset the machine through the keyboard controller (`0xFE` on port `0x64`) |
| `halt` | Stop the CPU (`cli` + `hlt`) |

## Defense guide

### GDT with the 6 required segments
Code: `gdt_init()` in `src/gdt.c`, constants in `include/gdt.h`.

<details>
<summary>Verify</summary>

`make run`, then **F3** (or type `gdt`): 7 entries, named kernel code / data / stack and user code / data / stack, after the null entry.
</details>

### GDT at `0x00000800`
`GDT_ADDRESS` in `include/gdt.h`. The table is written at runtime through a pointer to `0x800` (paging is off, so this is a physical address).

<details>
<summary>Verify</summary>

F3 shows `GDTR base 0x00000800 limit 0x0037`, read from the CPU with `sgdt`.

Independent check with the QEMU monitor:
```sh
qemu-system-i386 -cdrom kfs.iso -monitor stdio
(qemu) info registers      # line GDT= 00000800 00000037
(qemu) xp /7gx 0x800       # the 7 descriptors in physical memory
```
</details>

### GDT declared to the CPU
`src/gdt_flush.asm`: `lgdt`, then `mov` into DS/ES/FS/GS (`0x10`) and SS (`0x18`), then a far jump to reload CS (`0x08`). The CPU keeps a hidden copy of each segment descriptor: changing GDTR alone is not enough.

<details>
<summary>Verify</summary>

F3 shows `cs 0x0008 ds 0x0010 ss 0x0018`, and the accessed bit set on the kernel segments (`9b`, `93`).
</details>

### Kernel stack printer
Code: `src/stack.c`. Labels `stack_bottom` / `stack_top` are exported by `src/boot.asm`.

<details>
<summary>Verify</summary>

F2, or type `stack`: bounds, call trace ending with `end of chain (ebp = 0)`, raw dump. Match each return address with its function:
```sh
nm -n kernel.bin | grep -iE ' t '
```
An address belongs to the last symbol at or below it. From the shell, the trace reads `shell_execute`, `shell_handle_char`, `shell_run`, `kernel_main`, then `_start`.
</details>

### Flags, linker script, i386, size

<details>
<summary>Verify</summary>

```sh
make V=1 re | grep -- ' -c '                    # -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs ...
make V=1 re | grep -- '-T linker.ld'            # our own linker script
readelf -h kernel.bin | grep -E 'Class|Machine' # ELF32, Intel 80386
make check                                      # Multiboot header, size < 10 MB
```
`-fno-exceptions` and `-fno-rtti` are C++ options: the subject asks to adapt the flags to the language.
</details>

### Bonus: shell
Code: `src/shell.c`. A command table of `{name, help, function}` entries, searched with `strcmp` when Enter is pressed.

<details>
<summary>Verify</summary>

Type `help`, `stack`, `gdt`, an unknown word, then `reboot` (back to GRUB) and `halt` (`System halted.`, keyboard stops responding).
</details>

## Debugging with GDB

```sh
qemu-system-i386 -cdrom kfs.iso -s -S   # terminal 1
gdb -q kernel.bin                       # terminal 2
```

```
(gdb) target remote :1234
(gdb) hbreak kernel_main
(gdb) continue
(gdb) monitor info registers   # GDT= 00000800 00000037, CS=0008, SS=0018
(gdb) x/7gx 0x800              # our descriptors
(gdb) x/2wx $ebp               # saved EBP, return address (after the prologue)
```

## Design notes

- **GDT built at runtime** at a fixed address rather than placed by the linker: GRUB would otherwise have to load part of the ELF below 1 MiB.
- **Packed structures** (`gdt_entry` = 8 bytes, `gdt_ptr` = 6 bytes), checked with `_Static_assert`.
- **`xor ebp, ebp` in `_start`**: guarantees the frame chain ends at 0, whatever GRUB left in EBP.
- **`always_inline` on the register readers**: without `-O`, gcc does not inline, and a helper would read its own (already destroyed) frame.
- **Frame pointers** are kept because the kernel is built without `-O`; the stack walk relies on them.
