# neox — Task Management TODO

> Generated from deep source analysis of the task/ subsystem.
> Items are grouped by horizon: **Now** (bugs / correctness), **Near** (next milestone work),
> and **Far** (architectural evolution).
> Items already tracked in STATUS.md are marked *(tracked)*.

---

## NEAR — Next Milestone Work

Items the project already tracks in STATUS.md, listed here with
implementation notes from the code.

---

### Synchronization stress tests *(tracked)*

`scheduler_preemption_tests.c` is an empty file. The mutex and semaphore
tests only cover single-threaded scenarios (manually staging a waiter).
Needed:

- Multiple real threads racing on a shared mutex — verify the counter is
  always consistent.
- Semaphore used as a producer/consumer signal between two threads.
- `thread_wait` with a thread that is already terminated before the
  wait call (already handled by the `interrupt_save`-guarded check, but
  not tested).

---

### PMM linear-scan optimization *(tracked)*

The bitmap allocator does a full linear scan per allocation. Add a
`next_free_hint` index that starts at the last freed page, reducing
average-case cost from O(n) to O(1) amortized.

---

## FAR — Architectural Evolution

These require larger structural changes and depend on earlier work landing.

### Per-process address spaces

Currently all processes share `kernel_directory`. Each process needs its
own `struct page_directory *` stored in `struct process`. On context
switch, `cr3` must be reloaded — this plugs into `context_switch` or a
new `scheduler_switch_mm` hook called alongside it.

### Thread priorities

The round-robin scheduler has no concept of priority. A tiered ready queue
(one list per priority level, always pick from the highest non-empty tier)
is the simplest upgrade. Requires revisiting `scheduler_add` /
`scheduler_next`.

### Priority inheritance for mutexes

Without it, a high-priority thread blocked on a mutex held by a
low-priority thread can be starved indefinitely by medium-priority threads
(classic priority inversion). Requires priorities to exist first, then
tracking `mutex->owner` and temporarily boosting its priority to match the
highest-priority waiter.

### User mode (Ring 3) + system calls *(tracked)*

`thread_create` currently only creates kernel threads. User threads need:
- A user-mode stack.
- An entry point in Ring 3 (`iret` into Ring 3 with appropriate segment
  selectors, not a plain `ret` into `thread_bootstrap`).
- A syscall gate (`int 0x80` or `sysenter`) to re-enter the kernel.
- Per-thread `struct cpu_state` saved on the kernel stack by the syscall
  handler.

### ELF loader *(tracked)*

Depends on Ring 3 and a VFS/filesystem layer. Parses an ELF binary,
maps its segments into the process address space, and jumps to `e_entry`.

### SMP safety

All scheduler and sync primitives currently assume single-CPU execution.
Every `ASSERT(!interrupt_enabled())` guard would need to become a
spinlock acquisition. The `scheduler_add` / `scheduler_remove` list
manipulations would need per-CPU run queues or a global lock. This is a
distant but significant refactor — do not casually touch the interrupt
discipline model without a plan for it.

