# Changelog

All notable changes to Neox will be documented in this file.

---

## Unreleased

### Added

#### Boot

- Multiboot2 boot support.
- GRUB boot support.
- Kernel entry point.

#### Architecture

- Global Descriptor Table (GDT).
- Interrupt Descriptor Table (IDT).
- Interrupt Service Routines (ISR).
- Interrupt Request handling (IRQ).
- Programmable Interrupt Controller (PIC).
- Programmable Interval Timer (PIT).

#### Drivers

- VGA text-mode driver.
- PS/2 keyboard driver.
- QWERTY keymap.
- AZERTY keymap.

#### Memory

- Physical Memory Manager (PMM).
- Paging subsystem.
- Virtual Address Manager (VAM).
- Virtual Memory Manager (VMM).
- Kernel heap.

#### Tasking

- Process infrastructure.
- Thread infrastructure.
- Scheduler infrastructure.

#### Testing

- Generic kernel testing framework.
- PMM unit test suite.
- Paging unit test suite.
- VAM unit test suite.
- VMM unit test suite.
- Heap unit test suite.

### Changed

- Refactored paging table allocation into `paging_alloc_table()`.
- Propagated `PAGE_USER` permissions to page-directory entries.
- Refactored VMM page mapping and rollback into reusable private helpers.

### Fixed

- `paging_unmap()` is now idempotent when no page table exists.
- Added TLB invalidation after page mapping and unmapping.

## v0.5.0

### Added

- Process abstraction.
- Thread abstraction.
- Scheduler ready queue.
- x86 context switching.
- Cooperative kernel thread bootstrap.

### Tested

- Verified kernel → thread → kernel → thread context restoration.
- Validated preservation of execution state across context switches.
- Validated thread bootstrap and kernel thread execution.
