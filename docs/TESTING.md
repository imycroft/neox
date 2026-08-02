# Testing

Neox contains its own kernel testing framework.

Every subsystem must satisfy:

1. Unit tests
2. Stress tests

before being considered complete.

---

# Current Coverage

## PMM

### Unit Tests

- Allocate one page
- Allocate two pages
- Allocate three pages
- Allocate many pages
- Unique allocations
- Alignment
- Multiple alignment
- Free reuse
- Reverse free
- Lowest page reuse

### Stress Tests

- Allocate until Out Of Memory
- Recover after Out Of Memory

Result:

```
12 tests
12 passed
0 failed
```

---

## Paging

### Unit Tests

- Map one page
- Translate virtual address with offset
- Remap page
- Map many pages
- Directory boundary handling
- Translate unmapped address
- Unmap unmapped address
- Unmap one page without affecting others

### Stress Tests

- Allocate multiple page tables
- Map thousands of pages

Result:

```
10 tests
10 passed
0 failed
```

---

## VAM

### Unit Tests

- Allocate one page
- Page alignment
- Allocate two pages
- Allocate many pages
- Unique addresses
- Free reuse
- Reverse free
- Lowest page reuse
- Kernel space reservation

### Stress Tests

- Large virtual range allocation
- Large allocation free and reuse
- Repeated single-page allocation (manual stress test)

Result:

```
11 tests
11 passed
0 failed
```
Note:

The repeated single-page allocation stress test is not executed during normal kernel tests because the current first-fit allocator has linear scan behavior.

---

## VMM

### Unit Tests

- Allocate one page
- Page is mapped
- Allocate two pages
- Allocate many pages
- Unique physical pages
- Free reuse
- Reverse free
- Reallocate same virtual page
- Allocate pages anywhere
- Reject allocation over existing mapping
- Free unmapped page
- Reject zero-page allocation
- Reject zero-page automatic allocation
- Large allocation
- Large allocation free/reuse

Result:

```
15 tests
15 passed
0 failed
```

---

## Heap

### Unit Tests

- Allocate one byte
- Alignment
- Allocate two blocks
- Allocate many blocks
- Free reuse
- Reverse free
- Block merge
- Heap expansion
- Zero-byte allocation
- Large allocation
- Large allocation reuse
- Allocation/free cycle

### Stress Tests

- Allocate until Out Of Memory

Result:

```
13 tests
13 passed
0 failed
```

---

## Process

### Unit Tests

- Create process
- Unique PIDs
- No threads owned initially

Result:

```
3 tests
3 passed
0 failed
```

Note: no stress tests yet (e.g. sustained mass process creation). Process teardown/reclamation does not exist yet, so this subsystem is validated but not stress tested.

---

## Thread

### Unit Tests

- Create thread
- Initial state is THREAD_READY
- Kernel stack allocated and initial context within bounds
- Entry point stored
- Owning process association stored
- Unique TIDs

Result:

```
6 tests
6 passed
0 failed
```

Note: no stress tests yet. Thread teardown/reclamation does not exist yet (`thread_exit()` halts the thread in place rather than freeing it), so this subsystem is validated but not stress tested.

---

## Scheduler

### Unit Tests

- Empty ready list
- `scheduler_add(NULL)` is ignored
- Single-thread add and selection
- Round-robin selection order, including wraparound
- State transitions (THREAD_RUNNING / THREAD_READY) across selection
- Single-thread ready list repeats correctly

Result:

```
6 tests
6 passed
0 failed
```

Note: these are ready-queue unit tests only, run with interrupts disabled against stub threads (no real stack). They do not exercise an actual context switch. Preemptive round-robin scheduling itself (`scheduler_tick()` context switching between real, independently executing kernel threads) was validated by booting the kernel under QEMU and observing sustained, correctly interleaved execution of two kernel threads over hundreds of preemption cycles with no crashes — see the "Timer-driven preemptive scheduling" entry in the changelog. Stress tests (many threads, high-frequency `scheduler_yield()`, long-running sustained load) are still pending before this subsystem can be frozen.

---

# Development Rule

Subsystem lifecycle:

Design

↓

Implementation

↓

Unit Tests

↓

Stress Tests

↓

Refactor

↓

Freeze

Once frozen, a subsystem should only change to:

- fix bugs
- improve performance

Behavior must remain unchanged.
