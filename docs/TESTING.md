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
