NASM := nasm
GRUB := grub-mkrescue
QEMU := qemu-system-x86_64

# ------------------------------------------------------------
# General
# ------------------------------------------------------------

BUILD := build

CC := gcc
AS := nasm

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
         -Wpedantic \
         -Wshadow \
         -Wconversion \
         -Wsign-conversion \
         -Wcast-align \
         -Wundef \
         -Werror \
         -std=c23 \
         -Iinclude \
         -Ikernel/include \
         -c

CFLAGS += -DKERNEL_DEBUG

LDFLAGS := -T$(KERNEL_LD) -m elf_i386

ASFLAGS := -f elf32

# ------------------------------------------------------------
# User programs
#
# Layout:
#
#   user/
#   ├── init/
#   │   └── main.c
#   ├── hello/
#   │   └── main.c
#   └── ls/
#       └── main.c
#
# Produces:
#
#   build/user/init/main.o
#   build/user/init.elf
#
#   build/user/hello/main.o
#   build/user/hello.elf
#
#   build/user/ls/main.o
#   build/user/ls.elf
# ------------------------------------------------------------

USER_SOURCES := $(shell find user -mindepth 2 -maxdepth 2 -name 'main.c')

USER_PROGRAMS := \
    $(patsubst user/%/main.c,%,$(USER_SOURCES))

USER_OBJECTS := \
    $(addprefix $(BUILD)/user/,$(addsuffix /main.o,$(USER_PROGRAMS)))

USER_ELFS := \
    $(addprefix $(BUILD)/user/,$(addsuffix .elf,$(USER_PROGRAMS)))

USER_LD := user/user.ld

# ------------------------------------------------------------
# User compiler flags
# ------------------------------------------------------------

USER_CFLAGS = \
    -m32 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-pie \
    -fno-pic \
    -nostdlib \
    -nostdinc \
    -Wall \
    -Wextra \
    -mno-sse \
    -mno-sse2 \
    -std=c23 \
    -Werror \
    -Iinclude

# ------------------------------------------------------------
# Filesystem tool
# ------------------------------------------------------------

MKFS_SOURCE := tools/mkfs.c
MKFS_BUILD := $(BUILD)/tools
MKFS := $(MKFS_BUILD)/mkfs

MKFS_CFLAGS = \
    -std=c23 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror \
    -Iinclude

# ------------------------------------------------------------
# ISO
# ------------------------------------------------------------

ISO_BOOT := iso/boot
ISO_KERNEL := $(ISO_BOOT)/kernel.elf
ISO_ROOTFS := $(ISO_BOOT)/rootfs.img

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
# User programs
# ------------------------------------------------------------

$(BUILD)/user/%/main.o: user/%/main.c
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Link user programs
# ------------------------------------------------------------

$(BUILD)/user/%.elf: $(BUILD)/user/%/main.o $(USER_LD)
	@mkdir -p $(dir $@)
	ld -m elf_i386 -T$(USER_LD) -o $@ $<

# ------------------------------------------------------------
# Filesystem tool
# ------------------------------------------------------------

$(MKFS): $(MKFS_SOURCE) include/fs_format.h include/fs_types.h
	@mkdir -p $(dir $@)
	$(CC) $(MKFS_CFLAGS) $< -o $@

# ------------------------------------------------------------
# Filesystem
#
# Every:
#
#   build/user/hello.elf
#
# becomes:
#
#   /sbin/hello
# ------------------------------------------------------------

USER_FS_ENTRIES := \
    $(foreach elf,$(USER_ELFS),\
        /sbin/$(basename $(notdir $(elf)))=$(elf))

$(ISO_ROOTFS): $(MKFS) $(USER_ELFS)
	@mkdir -p $(dir $@)
	$(MKFS) $@ $(USER_FS_ENTRIES)

# ------------------------------------------------------------
# Create boot ISO
# ------------------------------------------------------------

$(IMAGE): $(ISO_KERNEL) $(ISO_ROOTFS) $(GRUB_CFG)
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
	rm -f $(ISO_BOOT)/rootfs.img
