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

---

## Process

### Unit Tests

- Create process
- Unique PIDs
- Process lookup by PID  (existing / missing)
- Process lookup by name (existing / missing)
- No threads owned initially

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

---

## Thread Integration

### Integration Tests

- Add thread through public thread API
- Execute newly created thread through real context switching
- Automatic thread termination after entry return
- Multiple threads yielding cooperatively
- Blocking and resuming threads
- Waiting for thread termination and free its memory (thread_join)

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

---

## Wait Queues

### Unit Tests

- FIFO add/remove
- Wake first waiting thread
- Wake one thread while preserving FIFO order
- Wake all waiting threads

**Note**

The wait queue subsystem provides the generic blocking infrastructure used by synchronization primitives.

---

## Mutex

### Unit Tests

- mutex init
- mutex lock
- mutex unlock
- mutex unlock transfers owner

**Note**

The mutex subsystem provides mutual exclusion using an owner field and a wait queue.

---

## Semaphore

- semaphore init
- semaphore acquire
- semaphore release
- semaphore release wakes

**Note**

The semaphore subsystem provides synchronization using a semaphore count and a wait queue.

---

## Condition Variable

- condvar_signal
- condvar_broadcast
- condvar_no_loss

**Note**

The condition variable subsystem provides:

- Waiting while atomically releasing the associated mutex
- Re-acquiring the mutex before returning from `condvar_wait()`
- Signaling one waiting thread
- Broadcasting to all waiting threads

---

# Test Organization

The current test suite is organized as follows:

```text
kernel/test/
├── condvar_tests.c
├── heap_tests.c
├── list_tests.c
├── mutex_tests.c
├── paging_tests.c
├── pmm_tests.c
├── process_tests.c
├── scheduler_tests.c
├── semaphore_tests.c
├── string_tests.c
├── test.c
├── tests.c
├── thread_integration_tests.c
├── thread_preemption_tests.c
├── thread_tests.c
├── vam_tests.c
├── vmm_tests.c
└── wait_tests.c
```

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
