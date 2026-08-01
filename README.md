# Neox

Neox is a 32-bit x86 hobby operating system written from scratch in C and x86 Assembly.

The project focuses on learning operating system internals while following modern software engineering practices:

- Clean architecture
- Incremental development
- Extensive testing
- Well-documented code
- Small, reviewable commits

---

---

## Goals

- Learn operating system internals by building a kernel from scratch.
- Emphasize correctness over feature count.
- Develop each subsystem through design, implementation, testing, refactoring, and freeze.
- Keep the codebase simple, readable, and maintainable.

## Implemented Subsystems

- Boot
- CPU initialization (GDT, IDT, ISR, IRQ, PIC, PIT)
- VGA text console
- Keyboard driver (QWERTY and AZERTY)
- Physical Memory Manager (PMM)
- Paging
- Virtual Address Manager (VAM)
- Virtual Memory Manager (VMM)
- Kernel Heap
- Process, Thread, and Scheduler infrastructure

---

## Testing

Neox contains a built-in kernel testing framework.

Current test suites:

- Physical Memory Manager (PMM)
- Paging

Additional subsystem tests will be added as each subsystem reaches the validation phase.

---

## Documentation

Project documentation is located in the `docs/` directory.

- STATUS.md
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

---

## Project Status

Neox is under active development.

For the current development status, completed milestones, and roadmap, see `docs/STATUS.md`.

---

## Repository Layout

```
kernel/    Kernel source code
docs/      Documentation
iso/       ISO image
```
---

## License

This project is licensed under the MIT License.
