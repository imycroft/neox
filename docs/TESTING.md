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
