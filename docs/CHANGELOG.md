```markdown
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
- x86 context switching.

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

- Process abstraction.
- Thread abstraction.
- Scheduler ready queue.
- Round-robin scheduler.
- Timer-driven preemptive scheduling.
- Scheduler-owned idle thread.
- Scheduler-owned reaper thread.
- Thread bootstrap.
- Thread blocking (`thread_block()`).
- Thread unblocking (`thread_unblock()`).
- Thread termination (`thread_exit()`).
- Joinable threads.
- Detached threads.
- Thread joining (`thread_join()`).
- Thread detachment.
- Thread destruction and resource reclamation.
- Process destruction after the final thread is removed.

#### Synchronization

- Generic wait queue infrastructure.
- FIFO wait queues.
- Wake-one (`wait_queue_wake()`).
- Wake-all (`wait_queue_wake_all()`).
- Semaphore implementation using wait queues.
- Semaphore acquire and release operations.
- Mutex implementation using wait queues.
- Mutex ownership tracking.
- Mutex ownership transfer.
- Condition variable implementation using wait queues and mutexes.
- Condition variable wait, signal, and broadcast operations.

#### Testing

- Generic kernel testing framework.
- PMM test suite.
- Paging test suite.
- VAM test suite.
- VMM test suite.
- Heap test suite.
- Process test suite.
- Thread test suite.
- Thread integration test suite.
- Scheduler test suite.
- Thread preemption test suite.
- Scheduler preemption test suite.
- Wait queue test suite.
- Semaphore test suite.
- Mutex test suite.
- Condition variable test suite.
- List test suite.
- String test suite.

### Changed

#### Scheduler

- Replaced the manual cooperative test scaffold with `scheduler_start()`.
- Scheduler state transitions now correctly handle running, ready, blocked, and terminated threads.
- Blocking the current thread now safely removes it from the ready queue.
- Timer-driven preemption now uses scheduler quantum accounting.
- The scheduler now tracks terminated threads separately from runnable threads.
- Added scheduler-managed reaping of detached terminated threads.

#### Thread Lifecycle

- Thread lifetime is now explicitly divided between joinable and detached threads.
- Joinable threads remain available for `thread_join()` after termination.
- Detached terminated threads are reclaimed by the reaper.
- Thread destruction now releases the thread's kernel stack and thread object.
- Process destruction is deferred until the process has no remaining threads.

#### Wait Queues

- Wait queue operations now maintain ownership through `thread->wait_queue`.
- Synchronization primitives now use wait queues rather than directly manipulating scheduler state.

#### Interrupt Handling

- `irq_handler()` sends the PIC EOI before invoking `scheduler_tick()` for IRQ0 so that a context switch from the timer interrupt cannot prevent the PIC from receiving its EOI.

#### Memory Management

- `paging_alloc_table()` centralizes page-table allocation.
- VMM page mapping and rollback logic was refactored into reusable private helpers.
- `PAGE_USER` permissions are propagated to page-directory entries.

### Fixed

#### Paging

- `paging_unmap()` safely handles an absent page table.
- Added TLB invalidation after page mapping and unmapping.

#### Threading

- `thread_bootstrap()` re-enables interrupts with `sti` on a thread's first execution.
- Fixed scheduler state corruption when blocking the current thread.
- Fixed handling of ready-list nodes when a thread blocks.
- Fixed wait queue ownership when a thread leaves a wait queue.
- Fixed terminated threads remaining eligible for scheduling.
- Fixed thread resource reclamation for joinable and detached threads.
- Fixed process lifetime handling when the final thread is destroyed.

---

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
```
