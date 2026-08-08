# Contributing

Neox is under active development.

The project follows an incremental development model: each subsystem is designed, implemented, thoroughly tested, refactored, and then frozen before work begins on the next subsystem.

---

# Current Contribution Policy

Feature contributions are **temporarily closed**.

Neox is still in the early stages of development, where the architecture of each subsystem is evolving rapidly. Because development follows a strict incremental workflow, multiple people cannot effectively implement the same subsystem in parallel without causing conflicting designs, duplicated work, or unnecessary architectural churn.

At this stage, the only external contributions being accepted are:

- Bug fixes
- Documentation corrections
- Typographical fixes

Bug-fix pull requests should remain focused on a single issue and should include regression tests whenever practical.

Once the kernel reaches a more mature and stable state, feature contributions will be opened. Development will then be organized around milestones and individual tasks so contributors can work on independent features in parallel.

---

# Development Workflow

Every subsystem follows the same lifecycle:

```text
Design
   ↓
Implementation
   ↓
Unit Tests
   ↓
Stress Tests
   ↓
Refactoring
   ↓
Freeze
```

Once a subsystem is **Frozen**, behavioral changes are no longer accepted unless they fix a bug. New functionality should be implemented by introducing new subsystems rather than modifying stable ones.

---

# Coding Style

- C23
- 4-space indentation
- Opening brace on the next line
- Descriptive function and variable names
- One responsibility per function
- Prefer simplicity over cleverness
- Avoid unnecessary abstractions
- No dynamic behavior hidden behind macros

---

# Design Principles

Neox favors simple, explicit designs.

When introducing new code:

- Keep interfaces small.
- Minimize dependencies between subsystems.
- Prefer composition over special cases.
- Avoid premature optimization.
- Avoid implementing features before a concrete use case exists.

Every subsystem should remain independently understandable and testable.

---

# Commits

Keep commits small and focused.

Each commit should represent one logical change.

Examples:

- Implement PMM
- Add PMM unit tests
- Refactor heap allocator
- Implement wait queues
- Add wait queue unit tests

Avoid mixing unrelated changes in the same commit.

---

# Testing

Every new subsystem must include tests.

A subsystem is not considered complete until it has:

- Unit tests
- Stress tests

Bug fixes should include regression tests whenever practical.

---

# Documentation

Documentation is maintained alongside the source code.

When a subsystem changes, update the relevant documentation:

- Architecture
- Status
- Roadmap
- Testing
- Changelog

Documentation should describe the current implementation, not future intentions.

---

# Philosophy

Neox values:

1. Correctness
2. Simplicity
3. Maintainability
4. Performance

Optimization is only performed after correctness has been established and must never change observable behavior.
