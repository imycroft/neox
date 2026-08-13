NASM := nasm
GRUB := grub-mkrescue
QEMU := qemu-system-x86_64

# Object files
BUILD := build


C_SOURCES := $(shell find kernel/test -name '*.c')
C_SOURCES += $(shell find kernel -name '*.c' ! -path 'kernel/test/*')


ASM_SOURCES := $(shell find kernel -name '*.asm')


OBJECTS := \
    $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SOURCES)) \
    $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

KERNEL_LD  := kernel/linker.ld
IMAGE := $(BUILD)/neox.iso

CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-pie -fno-pic -fno-builtin -fno-stack-protector -fno-omit-frame-pointer -nostartfiles -nodefaultlibs -ffreestanding -Wall -Wextra -std=c23 -Werror -Ikernel/include -c

LDFLAGS = -T$(KERNEL_LD) -m elf_i386


LDFLAGS += -Map=$(BUILD)/kernel.map

AS = nasm
ASFLAGS = -f elf32

ISO_BOOT := iso/boot
ISO_KERNEL := $(ISO_BOOT)/kernel.elf

# GRUB config

GRUB_CFG := iso/boot/grub/grub.cfg



.PHONY: all run clean

all: $(IMAGE)

# ------------------------------------------------------------
# Create build directory
# ------------------------------------------------------------

$(BUILD):
	mkdir -p $(BUILD)


# Kernel

$(ISO_KERNEL): $(OBJECTS) $(KERNEL_LD)
	ld $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: kernel/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
# ------------------------------------------------------------
# Create boot iso
# ------------------------------------------------------------

$(IMAGE): $(ISO_KERNEL) $(GRUB_CFG)
	$(GRUB) -o $@ iso -d /usr/lib/grub/i386-pc

# ------------------------------------------------------------
# Run
# ------------------------------------------------------------

run: $(IMAGE)
	$(QEMU) -m 256M -drive format=raw,file=$(IMAGE) -display curses -no-reboot -no-shutdown

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf $(BUILD)
	rm -f $(ISO_BOOT)/kernel.elf
