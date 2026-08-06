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
- Timer-driven preemptive round-robin scheduling (`scheduler_tick()`, `scheduler_yield()`, `scheduler_start()`).
- Scheduler-owned idle thread, replacing the ad hoc `main_sp` boot-stack pointer.
- Thread blocking (`thread_block()`).
- Thread unblocking (`thread_unblock()`).
- Thread termination (`thread_exit()`).
- Terminated thread tracking through the termination wait queue.
- Thread waiting (`thread_wait()`).
- Generic wait queue infrastructure.
- FIFO wait queues.
- Wake-one (`wait_queue_wake()`).
- Wake-all (`wait_queue_wake_all()`).

#### Synchronization

- Semaphore implementation using wait queues.
- Semaphore acquire/release operations.
- Semaphore wakeup behavior.
- Mutex implementation using wait queues.
- Mutex ownership tracking.
- Mutex ownership transfer on unlock.

#### Testing

- Generic kernel testing framework.
- PMM unit test suite.
- Paging unit test suite.
- VAM unit test suite.
- VMM unit test suite.
- Heap unit test suite.
- Process unit test suite.
- Thread unit test suite.
- Thread integration test suite.
- Scheduler unit test suite.
- Wait queue unit test suite.
- Semaphore test suite.
- Mutex test suite.
- Timer-driven preemption test suite.
- Quantum accounting tests.
- Multi-thread scheduler stress tests.

### Changed

- Refactored paging table allocation into `paging_alloc_table()`.
- Propagated `PAGE_USER` permissions to page-directory entries.
- Refactored VMM page mapping and rollback into reusable private helpers.
- `irq_handler()` now sends the PIC EOI before calling `scheduler_tick()` on IRQ0, since `scheduler_tick()` may context switch and suspend the current handler invocation indefinitely; sending EOI first keeps the timer firing for whichever thread runs next.
- Removed the manual cooperative test scaffold from `kernel_init()` (`thread1`, `main_sp`); replaced with `scheduler_start()`.
- Refactored scheduler state transitions to correctly handle blocked and terminated threads.
- Scheduler now safely handles removal of the currently running thread during blocking.
- Scheduler quantum handling now supports timer-driven preemption and quantum reset after scheduling decisions.
- Wait queue operations now maintain thread ownership through `thread->wait_queue`.
- Synchronization primitives now use wait queues instead of direct scheduler manipulation.

### Fixed

- `paging_unmap()` is now idempotent when no page table exists.
- Added TLB invalidation after page mapping and unmapping.
- `thread_bootstrap()` now re-enables interrupts (`sti`) on a thread's first run. A thread's first run is reached via `context_switch()`'s `ret`, not `iret`; when the switch was triggered from inside the timer ISR, interrupts were left disabled for that thread indefinitely without this fix.
- Fixed scheduler state corruption when blocking the current thread.
- Fixed scheduler handling of detached ready-list nodes during thread blocking.
- Fixed wait queue ownership bookkeeping when threads leave a wait queue.
- Fixed terminated thread handling by removing terminated threads from scheduling and tracking them separately.

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
