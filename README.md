# Neox

Neox is a 32-bit x86 hobby operating system written from scratch in C23 and x86 Assembly.

The project focuses on learning operating system internals while following disciplined software engineering practices:

- Clean architecture
- Incremental development
- Extensive testing
- Comprehensive documentation
- Small, focused commits

---

## Goals

- Learn operating system internals by building a kernel from scratch.
- Prioritize correctness, simplicity, and maintainability.
- Develop each subsystem through design, implementation, testing, refactoring, and freeze.
- Build a modular kernel with independently testable subsystems.

---

## Implemented Subsystems

### Boot

- GRUB
- Multiboot2

### Architecture

- Global Descriptor Table (GDT)
- Interrupt Descriptor Table (IDT)
- Interrupt Service Routines (ISR)
- Interrupt Requests (IRQ)
- Programmable Interrupt Controller (PIC)
- Programmable Interval Timer (PIT)
- x86 context switching

### Drivers

- VGA text-mode console
- PS/2 keyboard driver
- QWERTY keymap
- AZERTY keymap

### Memory Management

- Physical Memory Manager (PMM)
- Paging
- Virtual Address Manager (VAM)
- Virtual Memory Manager (VMM)
- Kernel heap

### Tasking

- Process infrastructure
- Thread infrastructure
- Preemptive round-robin scheduler
- Thread blocking and unblocking

### Synchronization

- Generic wait queues
- Wake-one and wake-all operations

---

## Testing

Neox includes a built-in kernel testing framework.

Current test coverage includes:

- Physical Memory Manager (PMM)
- Paging
- Virtual Address Manager (VAM)
- Virtual Memory Manager (VMM)
- Kernel Heap
- Process infrastructure
- Thread infrastructure
- Scheduler
- Wait queues

Every subsystem is validated through unit tests before being considered complete. Stress tests are added where appropriate before a subsystem is frozen.

---

## Documentation

Project documentation is located in the `docs/` directory.

- `STATUS.md`
- `ARCHITECTURE.md`
- `ROADMAP.md`
- `TESTING.md`
- `CHANGELOG.md`
- `CONTRIBUTING.md`

---

## Build

```bash
make
```

Run under QEMU:

```bash
make run
```

---

## Project Status

Neox is under active development.

Current focus:

- Kernel synchronization primitives
- Scheduler validation
- Thread and process lifecycle management

See `docs/STATUS.md` for the latest development status, completed milestones, and upcoming work.

---

## Repository Layout

```text
boot/       Bootloader
docs/       Project documentation
include/    Public kernel headers
kernel/     Kernel source code
iso/        Bootable ISO image
```

---

## License

This project is licensed under the MIT License.
