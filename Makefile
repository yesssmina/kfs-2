# Makefile - builds the KFS kernel and its bootable GRUB ISO image
# Usage: make [all|run|check|info|clean|fclean|re]   (add V=1 to see raw commands)

NAME		= kfs.iso
KERNEL		= kernel.bin

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= include
ISO_DIR		= iso
GRUB_DIR	= /usr/lib/grub/i386-pc
GRUB_LOG	= $(OBJ_DIR)/grub-mkrescue.log

# Toolchain: host binaries, but none of the host libraries or linker script
CC			= gcc
AS			= nasm
LD			= ld

# Subject flags adapted to C (-fno-rtti / -fno-exceptions are C++ only)
CFLAGS		= -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
			  -nostdlib -nodefaultlibs -nostdinc \
			  -Wall -Wextra -Werror -I$(INC_DIR)
ASFLAGS		= -f elf32
LDFLAGS		= -m elf_i386 -z noexecstack -T linker.ld
# BIOS modules only, no fonts/locales/themes, compressed: keeps the image small
GRUBFLAGS	= -d $(GRUB_DIR) --fonts= --locales= --themes= --compress=xz

ASM_SRCS	= boot.asm gdt_flush.asm
C_SRCS		= kernel.c \
			  printk.c \
			  string.c \
			  keyboard.c \
			  vga.c \
			  gdt.c \
			  stack.c

OBJS		= $(addprefix $(OBJ_DIR)/, $(ASM_SRCS:.asm=.o) $(C_SRCS:.c=.o))
DEPS		= $(OBJS:.o=.d)

# Use KVM acceleration only if this machine provides it
KVM			:= $(shell [ -w /dev/kvm ] && echo -enable-kvm)

# Subject limit: 10 MB, read strictly as 10 000 000 bytes (SI megabytes)
MAX_SIZE	= 10000000

# ---- Display ----
# V=1 shows the raw commands; by default only the short colored lines are shown
ifeq ($(V),1)
	Q =
else
	Q = @
endif

RESET		= \033[0m
BOLD		= \033[1m
RED			= \033[1;31m
GREEN		= \033[1;32m
YELLOW		= \033[1;33m
BLUE		= \033[1;34m
CYAN		= \033[1;36m

# $(call step,TAG,COLOR,TEXT): one aligned, colored progress line
step		= printf "  $(2)%-6s$(RESET) %s\n" "$(1)" "$(3)"

# $(call to_mb,BYTES): convert a byte count to MB (1 MB = 1 000 000 bytes)
to_mb		= awk -v b="$(1)" 'BEGIN { printf "%.2f MB", b / 1000000 }'

# ---- Rules ----
all: $(NAME)

# Bootable image: GRUB + our kernel + GRUB config
# grub-mkrescue runs in the background while a spinner shows the elapsed time
$(NAME): $(KERNEL) grub.cfg
	$(Q)mkdir -p $(ISO_DIR)/boot/grub
	$(Q)cp $(KERNEL) $(ISO_DIR)/boot/$(KERNEL)
	$(Q)cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(if $(Q),,@echo "grub-mkrescue $(GRUBFLAGS) -o $@ $(ISO_DIR)")
	@grub-mkrescue $(GRUBFLAGS) -o $@ $(ISO_DIR) > $(GRUB_LOG) 2>&1 & pid=$$!; \
	ticks=0; \
	while kill -0 $$pid 2>/dev/null; do \
		if [ -t 1 ]; then \
			case $$((ticks % 4)) in 0) s='|';; 1) s='/';; 2) s='-';; *) s='\';; esac; \
			printf "\r  $(CYAN)%-6s$(RESET) %s  %s %d.%ds " "ISO" "$(ISO_DIR)/ -> $@" \
				"$$s" $$((ticks / 10)) $$((ticks % 10)); \
		fi; \
		sleep 0.1; ticks=$$((ticks + 1)); \
	done; \
	wait $$pid; status=$$?; \
	[ -t 1 ] && printf "\r\033[K"; \
	if [ $$status -ne 0 ]; then \
		printf "  $(RED)%-6s$(RESET) grub-mkrescue failed:\n" "ISO"; cat $(GRUB_LOG); exit 1; \
	fi; \
	files=$$(grep -o '[0-9]* files added' $(GRUB_LOG) | tail -1 | cut -d' ' -f1); \
	sectors=$$(grep -o 'produced: [0-9]* sectors' $(GRUB_LOG) | grep -o '[0-9]*'); \
	printf "  $(CYAN)%-6s$(RESET) %s  $(GREEN)done$(RESET) in %d.%ds\n" "ISO" "$(ISO_DIR)/ -> $@" \
		$$((ticks / 10)) $$((ticks % 10)); \
	printf "         %s files (GRUB modules + kernel + config), %s sectors of 2 KB\n" \
		"$${files:-?}" "$${sectors:-?}"
	@size=$$(stat -c %s $@); \
	printf "\n  $(GREEN)$(BOLD)Build complete:$(RESET) $@ (%s)\n" "$$($(call to_mb,$$size))"
	@printf "  Next: $(BOLD)make run$(RESET) to boot, $(BOLD)make check$(RESET) to verify\n\n"

# Link every object with our own linker script
$(KERNEL): $(OBJS) linker.ld
	@$(call step,LD,$(BLUE),$(OBJS) -> $@)
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJS)

# C sources (-MMD: also track header dependencies in .d files)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(call step,CC,$(GREEN),$< -> $@)
	$(Q)$(CC) $(CFLAGS) -MMD -c $< -o $@

# Assembly sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(OBJ_DIR)
	@$(call step,ASM,$(YELLOW),$< -> $@)
	$(Q)$(AS) $(ASFLAGS) $< -o $@

# Boot the image in QEMU
run: $(NAME)
	@$(call step,QEMU,$(CYAN),booting $(NAME) $(if $(KVM),(KVM on),(KVM off)))
	$(Q)qemu-system-i386 $(KVM) -cdrom $(NAME)

# Defense checks: multiboot compliance and image size
check: $(NAME)
	@printf "\n  $(BOLD)Checks$(RESET)\n"
	@if grub-file --is-x86-multiboot $(KERNEL); then \
		printf "  $(GREEN)%-6s$(RESET) multiboot header\n" "OK"; \
	else \
		printf "  $(RED)%-6s$(RESET) multiboot header\n" "KO"; exit 1; \
	fi
	@size=$$(stat -c %s $(NAME)); \
	mb=$$($(call to_mb,$$size)); max=$$($(call to_mb,$(MAX_SIZE))); \
	if [ $$size -le $(MAX_SIZE) ]; then \
		printf "  $(GREEN)%-6s$(RESET) image size: $(BOLD)%s$(RESET) / %s  (%s bytes)\n" "OK" "$$mb" "$$max" "$$size"; \
	else \
		printf "  $(RED)%-6s$(RESET) image size: $(BOLD)%s$(RESET) / %s  (%s bytes)\n" "KO" "$$mb" "$$max" "$$size"; exit 1; \
	fi
	@printf "\n"

# Show the toolchain and flags (handy during the defense)
info:
	@printf "\n  $(BOLD)Toolchain$(RESET)\n"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "CC" "$$($(CC) --version | head -1)"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "AS" "$$($(AS) -v)"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "LD" "$$($(LD) --version | head -1)"
	@printf "\n  $(BOLD)Flags$(RESET)\n"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "CFLAGS" "$(CFLAGS)"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "ASFLAGS" "$(ASFLAGS)"
	@printf "  $(CYAN)%-9s$(RESET) %s\n" "LDFLAGS" "$(LDFLAGS)"
	@printf "  $(CYAN)%-9s$(RESET) %s\n\n" "GRUBFLAGS" "$(GRUBFLAGS)"

clean:
	@$(call step,CLEAN,$(RED),$(OBJ_DIR)/ $(ISO_DIR)/)
	$(Q)rm -rf $(OBJ_DIR) $(ISO_DIR)

fclean: clean
	@$(call step,CLEAN,$(RED),$(KERNEL) $(NAME))
	$(Q)rm -f $(KERNEL) $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all run check info clean fclean re
