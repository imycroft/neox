# Neox Status

**Last updated:** 2026-08-01

## Current Milestone

Memory management and cooperative kernel tasking completed.

The Physical Memory Manager (PMM), Paging, Virtual Address Manager (VAM), Virtual Memory Manager (VMM), Heap, Process infrastructure, Thread infrastructure, Scheduler infrastructure, and cooperative context switching have completed implementation, testing, and refactoring.

The next milestone is timer-driven preemptive scheduling.

---

## Subsystem Status

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Boot | ✅ Frozen | Complete and stable. |
| GDT | ✅ Frozen | Global Descriptor Table initialized. |
| IDT | ✅ Frozen | Interrupt Descriptor Table initialized. |
| ISR | ✅ Frozen | CPU exception handlers implemented. |
| IRQ | ✅ Frozen | Hardware interrupt handling implemented. |
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
| Process Infrastructure | ✅ Frozen | Process creation implemented and validated. |
| Thread Infrastructure | ✅ Frozen | Kernel thread creation and bootstrap validated. |
| Context Switching | ✅ Frozen | Cooperative kernel↔thread context switching validated. |
| Scheduler | 🚧 Implemented | Ready queue implemented; preemptive scheduling pending. |

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

---

## Next Milestone

Implement timer-driven preemptive scheduling.

After preemptive scheduling:

1. Introduce per-process address spaces.
2. Implement user mode.

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
