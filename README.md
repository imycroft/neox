# Neox

Neox is a hobby operating system written from scratch in C and x86 Assembly.

The project focuses on learning operating system internals while following modern software engineering practices:

- Clean architecture
- Incremental development
- Extensive testing
- Well-documented code
- Small, reviewable commits

---

## Current Features

### Boot

- Multiboot2
- GRUB bootloader
- ELF kernel
- Linker script

### Architecture

- Protected Mode
- GDT
- IDT
- ISR
- IRQ
- PIC
- PIT
- Exception handling

### Drivers

- VGA
- Keyboard
- QWERTY
- AZERTY

### Memory Management

- Physical Memory Manager (PMM)
- Paging
- Virtual Memory Manager (VMM)
- Virtual Address Manager (VAM)
- Kernel Heap

### Tasking

- Process infrastructure
- Thread infrastructure
- Scheduler infrastructure

---

## Testing

Current kernel contains a built-in testing framework.

Current subsystem coverage:

- Physical Memory Manager

Current status:

- 12 tests
- 0 failures

---

## Documentation

- ARCHITECTURE.md
- ROADMAP.md
- TESTING.md
- CHANGELOG.md
- CONTRIBUTING.md

---

## Build

```bash
make
```

Run:

```bash
make run
```

Debug:

```bash
make debug
```

---

## Project Status

Early development.

Current focus:

Memory management before multitasking.
