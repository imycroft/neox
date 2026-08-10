# Neox Architecture

Neox is divided into independent subsystems.

```text
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
Synchronization
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
- Context switching

---

# Memory Management

Layers:

```text
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

Responsible for physical page allocation.

## Paging

Virtual-to-physical address translation.

## VAM

Virtual address reservation.

## VMM

Maps virtual pages to physical pages.

## Heap

Dynamic kernel memory allocation.

---

# Tasking

Current components:

```text
Process
 │
 ▼
Thread
 │
 ▼
Scheduler
```

### Process

Owns resources and groups one or more threads.

### Thread

Represents the schedulable execution unit.

Each thread owns:

- Kernel stack
- Execution context
- Scheduling state

A thread belongs to exactly one process.

### Scheduler

Provides:

- Round-robin scheduling
- Timer-driven preemption
- Cooperative yielding
- Thread blocking
- Thread unblocking
- Thread termination
- Thread joining
- Detached thread reaping

---

# Synchronization

Current layers:

```text
Wait Queue
      │
      ├──► Semaphores
      │
      ├──► Mutexes
      │
      └──► Condition Variables
```

## Wait Queues

Generic FIFO blocking primitive.

Provides:

- Thread enqueue
- Wake one
- Wake all
- Blocked-thread ownership tracking

Forms the foundation for all higher-level synchronization primitives.

## Semaphores

Synchronization primitive implemented using wait queues.

Provides:

- Acquire
- Release
- Blocking when unavailable
- Wakeup of waiting threads

## Mutexes

Mutual exclusion primitive implemented using wait queues.

Provides:

- Lock
- Unlock
- Ownership tracking
- Ownership transfer to waiting threads

## Condition Variables

Condition synchronization primitive implemented using wait queues and mutexes.

Provides:

- Wait
- Signal
- Broadcast

A condition-variable wait atomically releases the associated mutex while blocking and re-acquires it before returning.

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

# Filesystems

Planned layers:

- VFS
- Initramfs
- Native filesystems

---

# Userland

Planned components:

- Ring 3
- System calls
- ELF loader
- libc
- Shell

---

# Philosophy

Each subsystem should be:

- Independent
- Testable
- Documented
- Refactorable
- Incrementally developed and frozen after validation
