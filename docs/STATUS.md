# Neox Status

**Last updated:** 2026-08-02

## Current Milestone

Timer-driven preemptive scheduling implemented and unit-tested.

The PIT timer now drives thread preemption: `scheduler_tick()` runs on every IRQ0, decrements the current thread's quantum, and switches to the next ready thread once it expires. The boot stack is registered with the scheduler as the idle thread via `scheduler_start()`, replacing the old manually-driven cooperative test scaffold in `kernel_init()`. Verified by booting under QEMU with two independent kernel threads and observing sustained, correctly-interleaved round-robin execution with no manual `context_switch()` calls anywhere in `kernel_init()`.

The next milestone is stress-testing the scheduler under sustained load, followed by Ring 3 (user mode).

---

## Subsystem Status

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Boot | ✅ Frozen | Complete and stable. |
| GDT | ✅ Frozen | Global Descriptor Table initialized. |
| IDT | ✅ Frozen | Interrupt Descriptor Table initialized. |
| ISR | ✅ Frozen | CPU exception handlers implemented. |
| IRQ | ✅ Frozen | Hardware interrupt handling implemented; IRQ0 now also drives `scheduler_tick()`, ordered after PIC EOI. |
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
| Process Infrastructure | ✅ Frozen | Process creation implemented and validated; unit test suite added (`process_tests.c`). |
| Thread Infrastructure | ✅ Frozen | Kernel thread creation and bootstrap validated; unit test suite added (`thread_tests.c`). Bug fix: `thread_bootstrap()` now re-enables interrupts on a thread's first run, required now that threads can be first scheduled from inside the timer ISR. |
| Context Switching | ✅ Frozen | Cooperative and preemptive kernel↔thread context switching validated. |
| Scheduler | 🚧 Implemented | Timer-driven preemptive round-robin scheduling implemented; unit test suite added (`scheduler_tests.c`, 6 cases). Stress testing (many threads, sustained tick/yield load, quantum tuning) still pending before freeze. |

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
- Process Infrastructure
- Thread Infrastructure

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

- `scheduler_next()` round-robins over every thread in the ready list regardless of `state`; it does not yet skip `THREAD_BLOCKED` or `THREAD_TERMINATED` threads.
- `thread_exit()` marks a thread `THREAD_TERMINATED` but its process/stack/tid are never reclaimed; the thread remains in the ready list and is scheduled forever, simply halting each time it runs.
- The quantum (`SCHEDULER_QUANTUM_TICKS`, currently 5 ticks / 50ms at 100Hz) is a single global value; there is no per-thread priority or dynamic quantum yet.
- These are acceptable for the current milestone but should be revisited once blocking primitives (locks, sleep, IPC) are introduced.

---

## Next Milestone

Stress-test the scheduler (many threads, sustained preemption, `scheduler_yield()` under load) and freeze it.

After the scheduler is frozen:

1. Ring 3 (user mode).
2. System calls.
3. ELF loader.
4. Introduce per-process address spaces.

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
