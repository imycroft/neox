NASM := nasm
GRUB := grub-mkrescue
QEMU := qemu-system-x86_64

# ------------------------------------------------------------
# General
# ------------------------------------------------------------

BUILD := build

CC = gcc
AS = nasm

# ------------------------------------------------------------
# Kernel sources
# ------------------------------------------------------------

C_SOURCES := $(shell find kernel -name '*.c')
ASM_SOURCES := $(shell find kernel -name '*.asm')

OBJECTS := \
    $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SOURCES)) \
    $(patsubst kernel/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

KERNEL_LD := kernel/linker.ld

CFLAGS = -m32 \
         -nostdlib \
         -nostdinc \
         -fno-builtin \
         -fno-stack-protector \
         -fno-omit-frame-pointer \
         -nostartfiles \
         -nodefaultlibs \
         -ffreestanding \
         -Wall \
         -Wextra \
         -std=c23 \
         -Werror \
         -Ikernel/include \
         -c

LDFLAGS = -T$(KERNEL_LD) -m elf_i386

ASFLAGS = -f elf32

# ------------------------------------------------------------
# User program
# ------------------------------------------------------------

USER_SOURCE := user/init.c
USER_LD := user/user.ld

USER_BUILD := $(BUILD)/user
USER_OBJECT := $(USER_BUILD)/init.o
USER_ELF := $(USER_BUILD)/init.elf

# ------------------------------------------------------------
# ISO
# ------------------------------------------------------------

ISO_BOOT := iso/boot
ISO_KERNEL := $(ISO_BOOT)/kernel.elf
ISO_INIT := $(ISO_BOOT)/init.elf

GRUB_CFG := iso/boot/grub/grub.cfg

IMAGE := $(BUILD)/neox.iso

# ------------------------------------------------------------
# Targets
# ------------------------------------------------------------

.PHONY: all run clean

all: $(IMAGE)

# ------------------------------------------------------------
# Kernel
# ------------------------------------------------------------

$(ISO_KERNEL): $(OBJECTS) $(KERNEL_LD)
	@mkdir -p $(dir $@)
	ld $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/%.o: kernel/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# ------------------------------------------------------------
# User program
# ------------------------------------------------------------

$(USER_OBJECT): $(USER_SOURCE)
	@mkdir -p $(dir $@)
	$(CC) \
		-m32 \
		-ffreestanding \
		-fno-stack-protector \
		-fno-pie \
		-fno-pic \
		-nostdlib \
		-nostdinc \
		-Wall \
		-Wextra \
		-std=c23 \
		-Werror \
		-c $< -o $@

$(USER_ELF): $(USER_OBJECT) $(USER_LD)
	@mkdir -p $(dir $@)
	ld -m elf_i386 -T$(USER_LD) -o $@ $(USER_OBJECT)

# Copy user ELF into ISO

$(ISO_INIT): $(USER_ELF)
	@mkdir -p $(dir $@)
	cp $< $@

# ------------------------------------------------------------
# Create boot ISO
# ------------------------------------------------------------

$(IMAGE): $(ISO_KERNEL) $(ISO_INIT) $(GRUB_CFG)
	@mkdir -p $(dir $@)
	$(GRUB) -o $@ iso -d /usr/lib/grub/i386-pc

# ------------------------------------------------------------
# Run
# ------------------------------------------------------------

run: $(IMAGE)
	$(QEMU) -m 256M \
		-drive format=raw,file=$(IMAGE) \
		-display curses \
		-no-reboot \
		-no-shutdown

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf $(BUILD)
	rm -f $(ISO_BOOT)/kernel.elf
	rm -f $(ISO_BOOT)/init.elf
