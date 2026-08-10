# Neox Status

**Last updated:** 2026-08-10

## Current Milestone

**Kernel synchronization primitives and thread lifecycle management implemented and tested.**

The scheduler now supports real thread blocking and unblocking. Threads may safely transition between `THREAD_RUNNING`, `THREAD_BLOCKED`, `THREAD_READY`, and `THREAD_TERMINATED` while maintaining scheduler state. Blocking removes the current thread from the scheduler's ready queue, while unblocking safely reintroduces it for future execution.

A generic wait queue subsystem has been implemented as the foundation for kernel synchronization primitives. Wait queues provide FIFO ordering, wake-one, and wake-all semantics and track the queue in which a blocked thread is sleeping.

Semaphores, mutexes, and condition variables have been implemented on top of the wait queue infrastructure. Condition variables atomically release the associated mutex while sleeping and re-acquire it before returning.

Thread termination and lifetime management are implemented through joinable and detached threads. Joinable threads are reclaimed by `thread_join()`, while detached terminated threads are placed on the scheduler's zombie list and reclaimed by the dedicated reaper thread. Process objects are reclaimed after their last thread is destroyed.

---

## Subsystem Status

| Subsystem                     | Status   | Notes                                                                                                                                                         |
| ----------------------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Boot                          | ✅ Frozen | Complete and stable.                                                                                                                                            |
| GDT                           | ✅ Frozen | Global Descriptor Table initialized.                                                                                                                            |
| IDT                           | ✅ Frozen | Interrupt Descriptor Table initialized.                                                                                                                         |
| ISR                           | ✅ Frozen | CPU exception handlers implemented.                                                                                                                             |
| IRQ                           | ✅ Frozen | Hardware interrupt handling implemented; IRQ0 drives `scheduler_tick()` after PIC EOI.                                                                        |
| PIC                           | ✅ Frozen | Legacy PIC initialized and remapped.                                                                                                                            |
| PIT                           | ✅ Frozen | System timer operational.                                                                                                                                       |
| Exception Handling            | ✅ Frozen | CPU exceptions handled correctly.                                                                                                                               |
| VGA Driver                    | ✅ Frozen | Text-mode console operational.                                                                                                                                  |
| Keyboard Driver               | ✅ Frozen | Keyboard input operational.                                                                                                                                     |
| QWERTY Keymap                 | ✅ Frozen | Complete.                                                                                                                                                       |
| AZERTY Keymap                 | ✅ Frozen | Complete.                                                                                                                                                       |
| Physical Memory Manager (PMM) | ✅ Frozen | Bitmap allocator with unit and stress test coverage.                                                                                                            |
| Paging                        | ✅ Frozen | Page mapping, translation, unmapping, and page-directory/page-table management implemented.                                                                     |
| Virtual Address Manager (VAM) | ✅ Frozen | Virtual address allocation and release validated with unit and stress tests.                                                                                    |
| Virtual Memory Manager (VMM)  | ✅ Frozen | Virtual reservation, physical backing, and page mapping implemented and tested.                                                                                  |
| Heap                          | ✅ Frozen | Kernel heap allocator validated.                                                                                                                                |
| Process Infrastructure        | ✅ Frozen | Process creation, process lookup by PID/name, thread ownership, and process destruction implemented.                                                            |
| Thread Infrastructure         | ✅ Frozen | Thread creation, bootstrap, blocking, unblocking, joining, detaching, termination, destruction, and integration tests completed.                               |
| Context Switching             | ✅ Frozen | Cooperative and preemptive kernel thread context switching validated.                                                                                           |
| Scheduler                     | ✅ Frozen | Round-robin preemptive scheduler completed with blocking support, quantum handling, zombie management, and comprehensive tests.                               |
| Wait Queues                   | ✅ Frozen | FIFO wait queues with wake-one, wake-all, and blocked-thread tracking implemented and tested.                                                                    |
| Semaphores                    | ✅ Frozen | Semaphore implementation completed using wait queues. Acquire, release, blocking, and wakeup behavior validated.                                               |
| Mutexes                       | ✅ Frozen | Mutex implementation completed using wait queues. Locking, unlocking, blocking, wakeup, and ownership behavior validated.                                       |
| Condition Variables           | ✅ Frozen | Condition variables implemented using wait queues and mutexes, including atomic release-and-sleep and mutex re-acquisition.                                     |
| Reaper                        | ✅ Frozen | Dedicated reaper thread reclaims detached terminated threads and destroys processes after their final thread is removed.                                         |

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
- Condition Variables
- Thread Join
- Thread Detachment
- Thread Termination
- Detached Thread Reaper
- Process Destruction

---

## Current Technical Debt

### Paging

- `paging_alloc_table()` currently assumes newly allocated page tables are identity mapped.
- When per-process address spaces are introduced, this helper will need to establish appropriate mappings before initializing page tables.

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

### Synchronization

- Synchronization stress testing can be expanded further beyond the current unit and integration coverage.

---

## Next Milestone

The next major milestone is moving from kernel-only task management toward user-mode execution.

Planned order:

1. Synchronization stress testing
2. Ring 3 (user mode)
3. System calls
4. ELF loader
5. Per-process address spaces

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
