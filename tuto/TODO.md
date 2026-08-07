# neox — Task Management TODO

> Generated from deep source analysis of the task/ subsystem.
> Items are grouped by horizon: **Now** (bugs / correctness), **Near** (next milestone work),
> and **Far** (architectural evolution).
> Items already tracked in STATUS.md are marked *(tracked)*.

---

## NOW — Bugs & Correctness Issues

These are real defects in the currently "frozen" subsystems.

### [BUG] `mutex_lock` fast path is not interrupt-safe

**File:** `kernel/task/mutex.c:20–24`

```c
// BROKEN — a tick can land between the check and the assignment
if (mutex->owner == NULL)
{
    mutex->owner = scheduler_current();
    return;
}
```

On a single-CPU kernel this is a real race: a PIT interrupt between the
`NULL` check and the assignment allows a second thread to observe `owner ==
NULL` and also claim ownership. Two threads can both hold the mutex.

**Fix:** wrap the entire fast path in `interrupt_save / interrupt_restore`.

```c
void mutex_lock(struct mutex *mutex)
{
    ASSERT(mutex != NULL);

    interrupt_state_t state = interrupt_save();

    if (mutex->owner == NULL)
    {
        mutex->owner = scheduler_current();
        interrupt_restore(state);
        return;
    }

    wait_queue_sleep(&mutex->wait_queue);

    interrupt_restore(state);
}
```

---

### [BUG] `semaphore_acquire` fast path is not interrupt-safe

**File:** `kernel/task/semaphore.c:20–24`

```c
// BROKEN — a tick between check and decrement causes underflow
if (sem->count > 0)
{
    sem->count--;
    return;
}
```

Two threads can both pass the `count > 0` check before either decrements.
On `uint32_t`, underflowing from 0 wraps to `UINT32_MAX` — silent and
catastrophic.

**Fix:** same pattern as mutex — interrupt-guard the whole function.

```c
void semaphore_acquire(struct semaphore *sem)
{
    ASSERT(sem != NULL);

    interrupt_state_t state = interrupt_save();

    if (sem->count > 0)
    {
        sem->count--;
        interrupt_restore(state);
        return;
    }

    wait_queue_sleep(&sem->wait_queue);

    interrupt_restore(state);
}
```

---

### [BUG] `mutex_unlock` missing interrupt guard

**File:** `kernel/task/mutex.c:34–53`

`wait_queue_remove`, the `owner` reassignment, and `thread_unblock` all run
with interrupts enabled. A tick between `remove` and `thread_unblock` leaves
a waiter stranded: removed from the queue but not yet in the ready list.
`thread_unblock` saves interrupts internally around `scheduler_add`, which
rescues the immediate crash — but `owner` can be observed inconsistent by
a concurrent `mutex_lock` fast path check.

**Fix:**

```c
void mutex_unlock(struct mutex *mutex)
{
    ASSERT(mutex != NULL);
    ASSERT(mutex->owner == scheduler_current());

    interrupt_state_t state = interrupt_save();

    struct thread *thread = wait_queue_remove(&mutex->wait_queue);

    if (thread != NULL)
    {
        mutex->owner = thread;
        interrupt_restore(state);      // restore before unblock (unblock saves its own)
        thread_unblock(thread);
        return;
    }

    mutex->owner = NULL;

    interrupt_restore(state);
}
```

---

### [BUG] `semaphore_release` missing interrupt guard

**File:** `kernel/task/semaphore.c:34–49`

`count++` in the no-waiter branch runs unguarded. A concurrent `acquire`
fast path can race with it, producing `count` values that don't correspond
to any real resource availability.

**Fix:** wrap the function body in `interrupt_save / interrupt_restore`,
mirroring `mutex_unlock`.

---

### [SMELL] `thread_exit` discards `interrupt_save` return value

**File:** `kernel/task/thread.c:20`

```c
interrupt_save();   // return value silently discarded
```

The intent is just to execute `cli` (the side effect of `interrupt_save`).
The discarded state means the original IF flag is never restored — correct
here because the thread is dying and will never return, but confusing to
read and will fire a `-Wunused-value` warning on stricter build flags.

**Fix:** replace with `interrupt_disable()` to document intent explicitly.

---

### [SMELL] `quantum_remaining` declared twice in `scheduler.c`

**File:** `kernel/task/scheduler.c:17` and `23`

```c
//DEBUG
static uint32_t quantum_remaining;   // line 17
//END DEBUG
static uint32_t quantum_remaining;   // line 23
```

Only one symbol exists (C merges them), so it compiles and works — but it
is confusing and will cause a pedantic-mode warning. Remove the debug copy
(lines 16–18) and the matching debug getter at lines 197–201, or move the
getter behind a compile-time `#ifdef NEOX_DEBUG` guard.

---

### [SMELL] `wait_queue_wake` / `wait_queue_wake_all` have no interrupt guard

**File:** `kernel/task/wait.c:63–85`

These functions call `thread_unblock` (which internally saves interrupts
around `scheduler_add`) but do not themselves require — or assert — that
the caller holds interrupts off. `scheduler_terminate` does call them
correctly under `ASSERT(!interrupt_enabled())`, but `mutex_unlock` and
`semaphore_release` call `thread_unblock` directly without that guarantee.

**Fix:** add `ASSERT(!interrupt_enabled())` to `wait_queue_wake` and
`wait_queue_wake_all`, matching the discipline of `wait_queue_sleep`.
This makes violations loud instead of silent.

---

## NEAR — Next Milestone Work

Items the project already tracks in STATUS.md, listed here with
implementation notes from the code.

### Condition variables *(tracked)*

The next planned primitive. The infrastructure is ready:
`wait_queue_sleep` / `wake` / `wake_all` are exactly the building
blocks needed. A `struct condvar` needs only an embedded `wait_queue`
and a paired `mutex *`. The tricky part is the **atomic release-and-sleep**
operation: the mutex must be released and the thread enqueued atomically
(under a single `interrupt_save`) to avoid the classic lost-wakeup bug.

Suggested signature:

```c
void condvar_wait(struct condvar *cv, struct mutex *mutex);
void condvar_signal(struct condvar *cv);
void condvar_broadcast(struct condvar *cv);
```

---

### Thread and process teardown *(tracked)*

`thread_exit` marks a thread `THREAD_TERMINATED` but never frees
`kernel_stack` or the `struct thread` itself. `process_create` has no
`process_destroy` counterpart.

Required cleanup sequence:

1. `thread_destroy(thread)` — `kfree(thread->kernel_stack)`, remove
   `group_node` from `process->threads`, `kfree(thread)`.
2. `process_destroy(process)` — iterate `process->threads`, call
   `thread_destroy` on each, remove `process_node` from `process_list`,
   `kfree(process)`.
3. A thread cannot free its own stack (it's executing on it). The
   canonical pattern is a **reaper thread**: terminated threads go onto
   a "zombie list" and a dedicated kernel thread drains and frees them.

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

