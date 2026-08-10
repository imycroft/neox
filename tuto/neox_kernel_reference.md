# Neox Kernel — Architecture & API Reference

---

## Global Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         kernel_main()                           │
│   kernel_init() → reaper_init() → process_create("init")       │
│                → thread_create() → thread_add() → kernel_loop() │
└───────────────────────────┬─────────────────────────────────────┘
                            │ PIT tick → scheduler_tick()
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      TASK SUBSYSTEM                             │
│                                                                 │
│  ┌──────────┐   owns    ┌──────────┐   scheduled by            │
│  │ process  │ ────────► │  thread  │ ──────────────────────┐   │
│  │  (pid)   │  threads  │  (tid)   │                       │   │
│  │  (name)  │           │  (state) │                       │   │
│  │  (list)  │           │  (stack) │                       ▼   │
│  └──────────┘           └──────────┘              ┌────────────┐│
│                              │                    │ scheduler  ││
│                         lifecycle                 │            ││
│                              │                    │ ready_list ││
│                              ▼                    │ zombie_list││
│                        ┌──────────┐               │ idle_thread││
│                        │  reaper  │ ◄─────────────│            ││
│                        │ (thread) │  zombie_list  └────────────┘│
│                        └──────────┘                             │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                   SYNCHRONIZATION PRIMITIVES                    │
│                                                                 │
│   wait_queue ◄── mutex ◄── condvar                             │
│   wait_queue ◄── semaphore                                      │
│   wait_queue ◄── thread.termination_queue (thread_join)        │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     MEMORY SUBSYSTEM                            │
│                                                                 │
│  pmm  (physical pages)                                          │
│   └► paging  (page directory / page tables)                     │
│       └► vam  (virtual address bitmap)                          │
│           └► vmm  (virtual memory allocator)                    │
│               └► heap  (kmalloc / kfree)                        │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     HARDWARE LAYER (x86)                        │
│                                                                 │
│  GDT → IDT → ISR stubs → irq_handler                           │
│  PIC (8259A) → IRQ routing                                      │
│  PIT (timer) → scheduler_tick() on every tick                   │
│  TSS (kernel stack on privilege switch)                         │
└─────────────────────────────────────────────────────────────────┘
```

---

## Boot Sequence

```
kernel.asm          — multiboot2 entry, sets up stack, calls kernel_main()
kernel_main()
  kernel_init()
    arch_init()     — GDT, IDT, ISR, IRQ, PIC, PIT, TSS
    pmm_init()      — scan multiboot2 memory map, build free-page bitmap
    paging_init()   — identity-map first 4 MB, enable paging
    vam_init()      — virtual address bitmap above 4 MB
    heap_init()     — kernel heap over vmm-backed pages
    scheduler_init()— init ready_list, zombie_list, quantum
    scheduler_start()— make boot stack the idle_thread, start quantum
  reaper_init()     — spawn reaper process + thread
  process_create("init") + thread_create() + thread_add()
  kernel_loop()     — idle (sti + hlt loop), preempted by PIT
```

---

## Thread Lifecycle

```
thread_create()
      │
      │  state = THREAD_READY
      │  kernel_stack allocated
      │  zombie_node = NULL/NULL
      │  detached = false
      ▼
thread_add()
      │
      ├── group_node  → process->threads
      └── sched_node  → scheduler ready_list

      │
      │  [scheduler picks it]
      ▼
   THREAD_RUNNING
      │
      ├─── thread_block()  ──► THREAD_BLOCKED
      │                              │
      │                        thread_unblock()
      │                              │
      │                        THREAD_READY ──► THREAD_RUNNING
      │
      └─── entry() returns ──► thread_exit() ──► scheduler_terminate()
                                                        │
                                              THREAD_TERMINATED
                                                        │
                                          ┌─────────────┴────────────┐
                                   joinable?                    detached?
                                     (detached=false)          (detached=true)
                                          │                          │
                                    thread_join()           zombie_list
                                          │                     │
                                   thread_destroy()         reaper_entry()
                                          │                     │
                                   process_destroy()      thread_destroy()
                                    (if last thread)            │
                                                         process_destroy()
                                                          (if last thread)
```

---

## Scheduler Internal State

```
ready_list   — doubly-linked list of THREAD_READY threads (via sched_node)
zombie_list  — doubly-linked list of terminated detached threads (via zombie_node)
current      — pointer to the currently running thread
idle_thread  — static thread, never in ready_list; runs when ready_list is empty
quantum_remaining — ticks left before preemption (resets to SCHEDULER_QUANTUM_TICKS)
```

Round-robin: `scheduler_next()` advances to `list_next(current->sched_node)`,
wrapping to `list_front()`. When `current` was removed (blocked/terminated) or
is `idle_thread`, jumps directly to `list_front()`.

---

## List Node Sentinel Convention

Every `list_node` uses `prev == NULL && next == NULL` to mean "not in any list".
`list_node_init()` sets both to NULL. `list_remove()` resets both to NULL after
unlinking. This convention is used by `scheduler_remove()`, `scheduler_next()`,
and `thread_destroy()` to safely guard against double-remove.

---

## API Reference

---

### `arch.h` — Interrupt Control

```c
typedef uint32_t interrupt_state_t;

// Save current interrupt flag (IF) and disable interrupts.
// Returns the saved state for interrupt_restore().
interrupt_state_t interrupt_save(void);

// Restore interrupt flag to the saved state.
void interrupt_restore(interrupt_state_t state);

// Enable interrupts unconditionally.
void interrupt_enable(void);

// Disable interrupts unconditionally.
void interrupt_disable(void);

// Enable interrupts and halt until the next interrupt (sti + hlt).
// Used by the idle thread to avoid busy-waiting.
void interrupt_enable_and_halt(void);

// Returns true if interrupts are currently enabled.
bool interrupt_enabled(void);
```

**Usage rule:** every call into the scheduler, thread, mutex, semaphore,
condvar, or wait_queue API must be made with interrupts disabled, OR the
function itself will save/disable/restore. Check each API entry below.

---

### `heap.h` — Kernel Heap

```c
// Allocate size bytes from the kernel heap. Returns NULL on failure.
// 8-byte aligned. Not ISR-reentrant — always call with interrupts disabled
// or from a thread context where no ISR can call kmalloc concurrently.
void *kmalloc(uint32_t size);

// Free a pointer previously returned by kmalloc.
void kfree(void *ptr);

// Dump heap free-list to the console (debug).
void heap_dump(void);
```

---

### `list.h` — Intrusive Doubly-Linked List

```c
struct list_node { struct list_node *prev, *next; };
struct list      { struct list_node head; };        // sentinel head

// Must be called before inserting a node into any list.
// Sets prev = next = NULL (the "not in list" sentinel).
void list_node_init(struct list_node *node);

void list_init(struct list *list);
bool list_empty(const struct list *list);

void list_push_front(struct list *list, struct list_node *node);
void list_push_back (struct list *list, struct list_node *node);
void list_insert_before(struct list_node *pos,  struct list_node *node);
void list_insert_after (struct list_node *pos,  struct list_node *node);

// Remove node from its list. Resets node to NULL/NULL sentinel.
void list_remove(struct list_node *node);

struct list_node *list_front(struct list *list);
struct list_node *list_back (struct list *list);
struct list_node *list_next (struct list_node *node);
struct list_node *list_prev (struct list_node *node);
```

**Pattern — recover the owning struct from a node:**
```c
// Given a list_node *node that is the sched_node field of a struct thread:
struct thread *t = container_of(node, struct thread, sched_node);
```

---

### `wait.h` — Wait Queue

The primitive on which mutex, semaphore, condvar, and `thread_join` are all built.

```c
struct wait_queue { struct list threads; };

void wait_queue_init(struct wait_queue *queue);

// Add a thread to the queue (does NOT block).
void wait_queue_add(struct wait_queue *queue, struct thread *thread);

// Remove and return the front thread (does NOT unblock it).
struct thread *wait_queue_remove(struct wait_queue *queue);

// Wake the front thread (unblock it, make it THREAD_READY).
// No-op if queue is empty.
void wait_queue_wake(struct wait_queue *queue);

// Wake every thread in the queue.
void wait_queue_wake_all(struct wait_queue *queue);

// Block the current thread on this queue.
// PRECONDITION: interrupts must be disabled by the caller.
// The function enqueues current, removes it from the scheduler,
// and yields. Interrupts remain disabled on return (the caller
// is responsible for restoring them).
void wait_queue_sleep(struct wait_queue *queue);
```

**Typical pattern:**
```c
interrupt_state_t state = interrupt_save();  // disable
wait_queue_sleep(&some_queue);               // block; re-enabled internally
interrupt_restore(state);                    // restore
```

---

### `mutex.h` — Mutual Exclusion

```c
struct mutex { struct thread *owner; struct wait_queue wait_queue; };

void mutex_init(struct mutex *mutex);

// Acquire the mutex. Blocks if already held.
// Manages interrupt state internally — safe to call with interrupts on or off.
void mutex_lock(struct mutex *mutex);

// Release the mutex. Wakes one waiter if any.
// Manages interrupt state internally.
void mutex_unlock(struct mutex *mutex);
```

**Usage:**
```c
mutex_lock(&m);
// critical section
mutex_unlock(&m);
```

---

### `semaphore.h` — Counting Semaphore

```c
struct semaphore { uint32_t count; struct wait_queue wait_queue; };

// Initialize with an initial count.
void semaphore_init(struct semaphore *sem, uint32_t count);

// Decrement count. Blocks if count is 0 until another thread releases.
void semaphore_acquire(struct semaphore *sem);

// Increment count. Wakes one waiter if any.
void semaphore_release(struct semaphore *sem);
```

---

### `condvar.h` — Condition Variable

Always used together with a mutex. Solves the lost-wakeup problem by
atomically releasing the mutex and sleeping.

```c
struct condvar { struct wait_queue wait_queue; };

void condvar_init(struct condvar *cv);

// Atomically release mutex and sleep on cv.
// Re-acquires mutex before returning.
// PRECONDITION:  caller holds mutex.
// POSTCONDITION: caller holds mutex.
void condvar_wait(struct condvar *cv, struct mutex *mutex);

// Wake one thread waiting on cv (if any).
void condvar_signal(struct condvar *cv);

// Wake all threads waiting on cv.
void condvar_broadcast(struct condvar *cv);
```

**Usage:**
```c
mutex_lock(&m);
while (!condition)
    condvar_wait(&cv, &m);   // releases m, sleeps, re-acquires m
// condition is now true, m is held
mutex_unlock(&m);
```

**Signaller:**
```c
mutex_lock(&m);
condition = true;
condvar_signal(&cv);
mutex_unlock(&m);
```

---

### `process.h` — Process

A process is a named container owning a set of threads. It carries no
scheduling weight of its own — the scheduler operates only on threads.

```c
struct process {
    pid_t            pid;
    char             name[32];
    struct list_node process_node;  // link in global process_list
    struct list      threads;       // list of thread.group_node
};

// Allocate and initialize a process, add it to the global process list.
struct process *process_create(const char *name);

// Add/remove from global process_list (called by create/destroy).
void process_add(struct process *process);
void process_remove(struct process *process);

// Lookup by pid or name.
struct process *process_find(pid_t pid);
struct process *process_find_by_name(const char *name);

// Free the process. Must only be called after all its threads are destroyed.
// PRECONDITION: interrupts disabled, process->threads is empty.
void process_destroy(struct process *process);
```

---

### `thread.h` — Thread

```c
enum thread_state {
    THREAD_READY,       // in ready_list, waiting for CPU
    THREAD_RUNNING,     // currently on CPU
    THREAD_BLOCKED,     // sleeping in a wait_queue
    THREAD_TERMINATED,  // entry() returned, off the scheduler
};

struct thread {
    tid_t             tid;
    uintptr_t         kernel_sp;          // saved stack pointer
    void             *kernel_stack;       // base of allocated stack
    void            (*entry)(void);       // thread entry function
    enum thread_state state;
    struct process   *process;            // owning process

    struct list_node  group_node;         // link in process->threads
    struct list_node  sched_node;         // link in scheduler ready_list
    struct list_node  wait_node;          // link in a wait_queue
    struct list_node  zombie_node;        // link in scheduler zombie_list

    bool              detached;           // true: reaper owns lifetime
    struct wait_queue termination_queue;  // thread_join() sleeps here
    struct wait_queue *wait_queue;        // queue thread is sleeping in
};
```

**Creating and starting a thread:**
```c
struct process *p = process_create("myproc");
struct thread  *t = thread_create(p, my_entry_function);

interrupt_state_t s = interrupt_save();
thread_add(t);      // links group_node + sched_node, makes schedulable
interrupt_restore(s);
```

**Waiting for a thread (joinable — default):**
```c
thread_join(t);     // blocks until t terminates, then frees t and
                    // possibly p. t pointer is INVALID after this.
```

**Fire-and-forget (detached):**
```c
thread_detach(t);   // hands lifetime to the reaper.
                    // t pointer is INVALID after this.
```

**From inside a thread:**
```c
// Voluntarily give up the CPU (cooperative yield).
thread_yield();

// Block until explicitly unblocked by another thread.
thread_block();
```

**From outside a blocked thread:**
```c
interrupt_state_t s = interrupt_save();
thread_unblock(t);  // puts t back in ready_list
interrupt_restore(s);
```

**Full API:**
```c
struct thread *thread_create(struct process *process, void (*entry)(void));
void           thread_add    (struct thread *thread);   // interrupts disabled
void           thread_yield  (void);
void           thread_block  (void);
void           thread_unblock(struct thread *thread);   // interrupts disabled
void           thread_join   (struct thread *thread);
void           thread_detach (struct thread *thread);
void           thread_destroy(struct thread *thread);   // interrupts disabled; reaper/join only
```

---

### `scheduler.h` — Scheduler (Public)

```c
#define SCHEDULER_QUANTUM_TICKS 5   // ticks per thread before preemption

// Initialize the ready_list and zombie_list. Call before scheduler_start().
void scheduler_init(void);

// Insert thread into the ready_list.
// PRECONDITION: interrupts disabled.
void scheduler_add(struct thread *thread);

// Remove thread from the ready_list (if present).
// PRECONDITION: interrupts disabled.
void scheduler_remove(struct thread *thread);

// Return the currently running thread (may be idle_thread or NULL before start).
struct thread *scheduler_current(void);

// Advance to and return the next thread to run. Updates current.
struct thread *scheduler_next(void);

// Make the boot stack the idle thread and begin scheduling.
// Call once, after scheduler_init(), before the first PIT tick.
void scheduler_start(void);

// Called on every PIT tick. Decrements quantum; preempts on expiry.
void scheduler_tick(void);

// Returns true if the idle thread is currently running.
bool scheduler_idle(void);

// Pop and return the next zombie from zombie_list, or NULL.
// PRECONDITION: interrupts disabled.
struct thread *scheduler_next_zombie(void);

// Restore a specific thread as current (used by scheduler tests only).
void scheduler_restore(struct thread *thread);

// Debug: return ticks remaining in current quantum.
uint32_t scheduler_get_quantum_remaining(void);
```

---

### `scheduler_internal.h` — Scheduler (Internal)

Only included by `thread.c`, `wait.c`, `reaper.c`. Not for general use.

```c
// Yield the CPU to the next schedulable thread.
// PRECONDITION: interrupts disabled.
void scheduler_yield(void);

// Mark thread THREAD_TERMINATED, remove from ready_list,
// wake termination_queue waiters, push to zombie_list if detached.
// PRECONDITION: interrupts disabled.
void scheduler_terminate(struct thread *thread);

// Push a terminated detached thread onto zombie_list and wake the reaper.
// No-op if reaper is not yet initialized (early boot).
// PRECONDITION: interrupts disabled, thread->state == THREAD_TERMINATED.
void scheduler_push_zombie(struct thread *thread);
```

---

### `reaper.h` — Reaper Thread

```c
// Spawn the reaper kernel thread. Call before any detached thread can terminate.
// Called by kernel_main() before process_create("init").
void reaper_init(void);

// Returns the reaper's wait_queue (used by scheduler_push_zombie to wake it),
// or NULL before reaper_init() has run.
struct wait_queue *reaper_wait_queue(void);

// Returns the reaper's struct thread * (used by scheduler_terminate to
// ASSERT the reaper never accidentally destroys itself).
struct thread *reaper_thread_get(void);
```

The reaper loop:
```
while true:
    wait_queue_sleep(&reaper_wq)        // sleep until a zombie arrives
    while (zombie = scheduler_next_zombie()) != NULL:
        process = zombie->process
        thread_destroy(zombie)          // free stack + struct
        if list_empty(process->threads):
            process_destroy(process)
```

---

### `pmm.h` — Physical Memory Manager

```c
void  pmm_init(void);               // parse multiboot2 memory map
void *pmm_alloc_page(void);         // allocate one 4 KB physical page
void  pmm_free_page(void *page);    // free one physical page
uint32_t pmm_total_memory(void);    // total RAM in bytes
uint32_t pmm_usable_memory(void);   // usable RAM in bytes
uint32_t pmm_free_pages(void);      // current free page count
```

---

### `paging.h` — Paging

```c
void paging_init(void);                         // identity-map first 4 MB, enable CR0.PG
void paging_map(uintptr_t virt, uintptr_t phys, uint32_t flags);
void paging_unmap(uintptr_t virt);
uintptr_t paging_translate(uintptr_t virt);     // virtual → physical
void paging_load_directory(uint32_t directory); // load CR3
void paging_enable(void);                       // set CR0.PG
void paging_invalidate(uintptr_t address);      // invlpg

// Flags:
#define PAGE_PRESENT   (1 << 0)
#define PAGE_WRITABLE  (1 << 1)
#define PAGE_USER      (1 << 2)
```

---

### `vam.h` — Virtual Address Manager

Tracks which virtual pages above the 4 MB identity window are in use.

```c
void  vam_init(void);
void *vam_alloc_pages(uint32_t count);              // reserve count virtual pages
void  vam_free_pages(uintptr_t addr, uint32_t count);
void  vam_dump(void);                               // debug
```

---

### `vmm.h` — Virtual Memory Manager

Combines vam (virtual reservation) + pmm (physical backing) + paging (mapping).

```c
void *vmm_alloc_page (uintptr_t virt);              // map virt → new physical page
void  vmm_free_page  (uintptr_t virt);
void *vmm_alloc_pages(uintptr_t virt, uint32_t count);
void  vmm_free_pages (uintptr_t virt, uint32_t count);
void *vmm_alloc_pages_any(uint32_t count);          // find + map anywhere in kernel VA space
```

---

### `assert.h` / `panic.h`

```c
// Halt with file/line/expression if expr is false.
ASSERT(expr);

// Halt with a message string.
void panic(const char *message);
```

---

## Cross-Subsystem Rules

**Interrupt discipline**

| Caller context | Rule |
|---|---|
| ISR (timer, keyboard) | Interrupts already disabled by CPU on entry |
| Thread — calling scheduler/thread/sync API | `interrupt_save()` before, `interrupt_restore()` after |
| Thread — calling `mutex_lock`, `semaphore_acquire`, `condvar_wait` | Safe to call with interrupts on; function manages state internally |
| Thread — calling `thread_add`, `thread_unblock` | Must have interrupts disabled |
| Reaper — `thread_destroy`, `process_destroy` | Must have interrupts disabled; kept disabled for entire destroy sequence |

**Object lifetime rules**

| Situation | Correct action |
|---|---|
| Thread you created and want to wait for | `thread_join(t)` — blocks, then frees. Pointer invalid after return. |
| Thread you created but don't need to wait for | `thread_detach(t)` — reaper will free it. Pointer invalid after return. |
| Reading `t->state` after `thread_join(t)` | **Never** — pointer is freed. Save state before joining. |
| Freeing a thread from inside itself | **Never** — stack is still in use. Always deferred to join or reaper. |
| Calling `process_destroy` before all threads are destroyed | **Never** — ASSERT will fire. |

**Scheduler invariants**

| Invariant | Enforced by |
|---|---|
| `idle_thread` never in `ready_list` | `scheduler_start()` does not call `scheduler_add` |
| `idle_thread.state` never set to `THREAD_READY` | Guard in `scheduler_next()` |
| Reaper never in `zombie_list` | `ASSERT(thread != reaper_thread_get())` in `scheduler_terminate()` |
| `sched_node` NULL/NULL means "not in ready_list" | `list_node_init()` + `list_remove()` reset semantics |
| `zombie_node` NULL/NULL means "not in zombie_list" | Same convention |

---

## Header Dependency Map

```
types.h
  └── memory.h
        └── list.h
              └── wait.h
                    ├── mutex.h
                    ├── semaphore.h
                    ├── condvar.h  (also needs mutex.h)
                    └── thread.h
                          ├── process.h
                          ├── scheduler.h
                          ├── scheduler_internal.h  (internal only)
                          └── reaper.h
arch.h  (independent — only types.h)
heap.h  (independent — only types.h)
pmm.h → memory.h, types.h
paging.h → types.h
vam.h → memory.h, types.h
vmm.h → memory.h, types.h
panic.h  (no dependencies)
assert.h (no dependencies)
```

---

## Source File Map

```
kernel/
├── kernelmain.c          boot entry: init, reaper_init, spawn init thread
├── kernel/
│   ├── kernel.c          kernel_init(), kernel_loop()
│   └── panic.c
├── arch/x86/
│   ├── gdt.c / gdt_flush.asm
│   ├── idt.c / idt_load.asm
│   ├── isr.c / isr_stubs.asm
│   ├── irq.c / irq_stubs.asm
│   ├── pic.c
│   ├── pit.c             timer: pit_handler() → scheduler_tick()
│   ├── tss.c
│   ├── arch.c            arch_init()
│   ├── context.asm       context_switch(old_sp, new_sp)
│   ├── paging.asm
│   └── io.c              inb / outb
├── mm/
│   ├── pmm.c             physical page bitmap allocator
│   ├── paging.c          page directory / table management
│   ├── vam.c             virtual address bitmap
│   ├── vmm.c             virtual memory allocator
│   └── heap.c            kmalloc / kfree (first-fit free list)
├── task/
│   ├── scheduler.c       ready_list, zombie_list, round-robin, quantum
│   ├── thread.c          create/add/block/unblock/join/detach/destroy
│   ├── process.c         create/destroy, global process_list
│   ├── reaper.c          dedicated cleanup thread for detached threads
│   ├── wait.c            wait_queue_sleep/wake/wake_all
│   ├── mutex.c           binary mutex (owner + wait queue)
│   ├── semaphore.c       counting semaphore
│   └── condvar.c         condition variable (atomic release + sleep)
├── lib/
│   ├── list.c            intrusive doubly-linked list
│   ├── string.c          memset, memcpy, strcmp, strncpy, strlen
│   ├── printf.c          kprintf (VGA output)
│   ├── assert.c          assert_fail → panic
│   └── debug.c
├── drivers/
│   ├── vga.c             80×25 text mode
│   ├── keyboard.c        PS/2 keyboard ISR
│   └── keymap.c / keymaps/
└── test/                 unit and integration test suites
```
