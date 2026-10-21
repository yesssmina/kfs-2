#!/bin/sh
# install_deps.sh - installs and checks the tools needed to build and run KFS
# Target: Debian / Ubuntu (apt). Requires sudo.

set -e

PACKAGES="build-essential nasm grub-pc-bin grub-common xorriso mtools qemu-system-x86 file"

# 1. Make sure we are on an apt-based system
if ! command -v apt-get >/dev/null 2>&1; then
    echo "Error: apt-get not found (this script targets Debian/Ubuntu)." >&2
    exit 1
fi

# 2. Install packages
echo "==> Installing: $PACKAGES"
sudo apt-get update
sudo apt-get install -y $PACKAGES

# 3. Check that every tool is available
echo "==> Checking tools"
MISSING=0
for tool in gcc ld nasm make grub-mkrescue grub-file xorriso qemu-system-i386 file; do
    if command -v "$tool" >/dev/null 2>&1; then
        printf "  %-18s OK\n" "$tool"
    else
        printf "  %-18s MISSING\n" "$tool"
        MISSING=1
    fi
done

# 4. Check GRUB modules for BIOS boot (classic pitfall: ISO builds but won't boot)
if [ -d /usr/lib/grub/i386-pc ]; then
    echo "  GRUB BIOS modules  OK"
else
    echo "  GRUB BIOS modules  MISSING (package grub-pc-bin)"
    MISSING=1
fi

# 5. Check that gcc can produce i386 code
#    Errors are no longer hidden: if it fails, we want to see why.
TMP=$(mktemp -d)
echo 'void f(void){}' > "$TMP/t.c"
if gcc -m32 -ffreestanding -c "$TMP/t.c" -o "$TMP/t.o"; then
    # Older versions of `file` print "Intel 80386", newer ones "Intel i386"
    if file "$TMP/t.o" | grep -Eq "ELF 32-bit.*Intel (80386|i386)"; then
        echo "  gcc -m32 (i386)    OK"
    else
        echo "  gcc -m32 (i386)    FAILED: object is not i386 ->"
        file "$TMP/t.o"
        MISSING=1
    fi
else
    echo "  gcc -m32 (i386)    FAILED: compilation error (see above)"
    MISSING=1
fi
rm -rf "$TMP"

# 6. Print versions, to compare home VM and campus VM
echo "==> Versions"
gcc --version | head -1
nasm -v
ld --version | head -1

if [ "$MISSING" -ne 0 ]; then
    echo "==> Some requirements are missing, see above." >&2
    exit 1
fi
echo "==> Environment ready."
