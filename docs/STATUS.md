# Neox Status

**Last updated:** 2026-08-01

## Current Milestone

Memory subsystem validation.

The Physical Memory Manager (PMM), Paging subsystem, and Virtual Address Manager (VAM) have completed implementation, testing, stress testing, and refactoring.

The next milestone is validating the Virtual Memory Manager (VMM).

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
| Virtual Memory Manager (VMM) | 🚧 Pending Validation | Implemented but not yet validated. |
| Heap | 🚧 Pending Validation | Implemented but not yet validated. |
| Process Infrastructure | 🚧 Implemented | Context switching not yet implemented. |
| Thread Infrastructure | 🚧 Implemented | Context switching not yet implemented. |
| Scheduler | 🚧 Implemented | Context switching not yet implemented. |

---

## Completed Milestones

- Boot
- CPU Initialization
- Interrupt Handling
- Physical Memory Manager (PMM)
- Paging
- Virtual Address Manager (VAM)

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

## Next Milestone

Validate and freeze the Virtual Memory Manager (VMM).

After VMM:

1. Validate and freeze the Heap allocator.
2. Implement context switching.
3. Introduce per-process address spaces.
4. Implement user mode.

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
