# Neox Architecture

Neox is divided into independent subsystems.

```
Boot
 │
 ▼
Architecture
 │
 ▼
Memory Management
 │
 ▼
Tasking
 │
 ▼
Drivers
 │
 ▼
Filesystems
 │
 ▼
Userland
```

---

# Boot

Responsible for:

- GRUB
- Multiboot2
- Kernel loading
- Early initialization

---

# Architecture

Contains CPU-specific code.

Current components:

- GDT
- IDT
- ISR
- IRQ
- PIC
- PIT

---

# Memory Management

Layers:

```
PMM
 │
 ▼
Paging
 │
 ▼
VAM
 │
 ▼
VMM
 │
 ▼
Heap
```

## PMM

Bitmap allocator.

Responsible for physical pages.

## Paging

Virtual to physical translation.

## VAM

Virtual address reservation.

## VMM

Maps virtual pages to physical pages.

## Heap

Dynamic kernel allocations.

---

# Tasking

Current infrastructure:

- Process
- Thread
- Scheduler

Future:

- Context switching
- User mode
- Process isolation

---

# Drivers

Current:

- VGA
- Keyboard

Future:

- ATA
- AHCI
- PCI
- USB
- Network

---

# Philosophy

Each subsystem should be:

- Independent
- Testable
- Documented
- Refactorable
