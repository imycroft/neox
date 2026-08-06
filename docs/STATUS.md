# Neox Status

**Last updated:** 2026-08-06

## Current Milestone

**Kernel synchronization primitives implemented and tested.**

The scheduler now supports real thread blocking and unblocking. Threads may safely transition between `THREAD_RUNNING`, `THREAD_BLOCKED`, `THREAD_READY`, and `THREAD_TERMINATED` without corrupting scheduler state. Blocking removes the current thread from the scheduler's ready queue, while unblocking safely reintroduces it for future execution.

A generic wait queue subsystem has been implemented as the foundation for kernel synchronization primitives. Wait queues provide FIFO ordering, wake-one, and wake-all semantics while tracking queue ownership for blocked threads.

Semaphores and mutexes have been implemented on top of the wait queue infrastructure and validated through integration tests covering initialization, acquisition, release, blocking, wakeup behavior, and mutex ownership transfer.

The next milestone is implementing condition variables and completing the remaining thread lifecycle management features.

---

## Subsystem Status

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Boot | ✅ Frozen | Complete and stable. |
| GDT | ✅ Frozen | Global Descriptor Table initialized. |
| IDT | ✅ Frozen | Interrupt Descriptor Table initialized. |
| ISR | ✅ Frozen | CPU exception handlers implemented. |
| IRQ | ✅ Frozen | Hardware interrupt handling implemented; IRQ0 drives `scheduler_tick()` after PIC EOI. |
| PIC | ✅ Frozen | Legacy PIC initialized and remapped. |
| PIT | ✅ Frozen | System timer operational. |
| Exception Handling | ✅ Frozen | CPU exceptions handled correctly. |
| VGA Driver | ✅ Frozen | Text-mode console operational. |
| Keyboard Driver | ✅ Frozen | Keyboard input operational. |
| QWERTY Keymap | ✅ Frozen | Complete. |
| AZERTY Keymap | ✅ Frozen | Complete. |
| Physical Memory Manager (PMM) | ✅ Frozen | Bitmap allocator with complete unit and stress test suite. |
| Paging | ✅ Frozen | Mapping, translation, unmapping, remapping, and stress testing complete. |
| Virtual Address Manager (VAM) | ✅ Frozen | Virtual address allocation and release validated with unit and stress tests. |
| Virtual Memory Manager (VMM) | ✅ Frozen | Virtual-to-physical mapping validated. |
| Heap | ✅ Frozen | Kernel heap allocator validated. |
| Process Infrastructure | ✅ Frozen | Process creation, process lookup (PID and name), and unit tests completed. |
| Thread Infrastructure | ✅ Frozen | Thread creation, bootstrap, blocking, unblocking, waiting, termination, and integration tests completed. |
| Context Switching | ✅ Frozen | Cooperative and preemptive kernel↔thread context switching validated. |
| Scheduler | ✅ Frozen | Round-robin preemptive scheduler completed with blocking support, scheduler integration, quantum handling, stress testing, and comprehensive tests. |
| Wait Queues | ✅ Frozen | FIFO wait queues with wake-one, wake-all, ownership tracking, and comprehensive tests completed. |
| Semaphores | ✅ Frozen | Semaphore implementation completed using wait queues. Initialization, acquire, release, and wakeup behavior validated. |
| Mutexes | ✅ Frozen | Mutex implementation completed using wait queues. Locking, unlocking, and ownership transfer validated. |

---

## Completed Milestones

- Boot
- CPU Initialization
- Interrupt Handling
- Physical Memory Manager (PMM)
- Paging
- Virtual Address Manager (VAM)
- Virtual Memory Manager (VMM)
- Heap
- Cooperative Context Switching
- Preemptive Context Switching
- Process Infrastructure
- Thread Infrastructure
- Scheduler
- Wait Queue Infrastructure
- Semaphores
- Mutexes

---

## Current Technical Debt

### Paging

- `paging_alloc_table()` currently assumes newly allocated page tables are identity mapped.
- When per-process address spaces are introduced, this helper will become responsible for establishing temporary mappings before initializing page tables.

### Process Address Spaces

- All processes currently share `kernel_directory`.
- Per-process page directories will be introduced before user-mode support.

### Physical Memory Manager

- The bitmap allocator performs a linear scan to locate the next free page.
- A future optimization may introduce a `next_free_hint` to reduce allocation time without changing allocator behavior.

### Scheduler

- The scheduler currently implements a single round-robin scheduling policy.
- All runnable threads share the same execution quantum (`SCHEDULER_QUANTUM_TICKS`).
- Thread priorities and dynamic scheduling policies have not yet been introduced.

### Thread Lifecycle

- `thread_exit()` marks threads as terminated but does not yet reclaim kernel stacks or thread objects.
- Process termination does not yet terminate or reclaim all threads belonging to a process.
- Zombie thread/process handling has not yet been implemented.

### Synchronization

- Condition variables have not yet been implemented.
- Advanced synchronization stress testing remains to be added.

---

## Next Milestone

Implement condition variables and continue improving thread lifecycle management.

Planned order:

1. Condition Variables
2. Synchronization stress testing
3. Thread and process termination
4. Ring 3 (user mode)
5. System calls
6. ELF loader
7. Per-process address spaces

---

## Freeze Policy

A subsystem is considered **Frozen** only after completing the following phases:

1. Design
2. Implementation
3. Unit Tests
4. Stress Tests
5. Refactoring

Once frozen, a subsystem may only change to:

- Fix bugs.
- Improve performance.
- Refactor without changing behavior.

Behavioral changes require starting a new development cycle.
