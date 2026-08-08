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

```text
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

```text
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

```text
11 tests
11 passed
0 failed
```

**Note**

The repeated single-page allocation stress test is not executed during normal kernel tests because the current first-fit allocator performs a linear bitmap scan.

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

```text
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

```text
13 tests
13 passed
0 failed
```

---

## Process

### Unit Tests

- Create process
- Unique PIDs
- Process lookup by PID
- Process lookup by name
- No threads owned initially

Result:

```text
5 tests
5 passed
0 failed
```

**Note**

Process termination and resource reclamation are not yet implemented.

---

## Thread

### Unit Tests

- Create thread
- Initial state is `THREAD_READY`
- Kernel stack allocated and initial context within bounds
- Entry point stored
- Owning process association stored
- Unique TIDs
- Block thread
- Unblock thread
- Wait on queue

Result:

```text
9 tests
9 passed
0 failed
```

---

## Thread Integration

### Integration Tests

- Add thread through public thread API
- Execute newly created thread through real context switching
- Automatic thread termination after entry return
- Multiple threads yielding cooperatively
- Blocking and resuming threads
- Waiting for thread termination

Result:

```text
7 tests
7 passed
0 failed
```

---

## Scheduler

### Unit Tests

- Empty ready list
- `scheduler_add(NULL)` is ignored
- Single-thread add and selection
- Round-robin selection order (including wraparound)
- State transitions (`THREAD_RUNNING` ↔ `THREAD_READY`)
- Single-thread ready list repeats correctly
- Remove thread from ready queue
- Remove current blocked thread
- Current thread tracking

Result:

```text
9 tests
9 passed
0 failed
```

---

## Thread Preemption

### Preemptive Scheduling Tests

- Timer tick synchronization
- Timer-driven preemption of CPU-bound threads
- Round-robin scheduling without voluntary yielding
- Blocked thread exclusion from scheduling
- Multiple runnable threads under timer preemption
- Quantum initial value
- Quantum countdown
- Quantum reset after context switch
- Scheduler stress with 32 CPU-bound threads

Result:

```text
9 tests
9 passed
0 failed
```

---

## Wait Queues

### Unit Tests

- FIFO add/remove
- Wake first waiting thread
- Wake one thread while preserving FIFO order
- Wake all waiting threads

Result:

```text
4 tests
4 passed
0 failed
```

**Note**

The wait queue subsystem provides the generic blocking infrastructure used by synchronization primitives.

---

# Development Rule

Subsystem lifecycle:

```text
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
```

Once frozen, a subsystem should only change to:

- Fix bugs.
- Improve performance.

Behavior must remain unchanged.
