
## Exercise 9 — Counting semaphore for a buffer pool

**System name:** DMA buffer pool manager
**One-sentence description:** Multiple tasks request, use, and return one of 4 fixed DMA buffers from a shared pool.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Request a buffer | Task needs one | Should not block indefinitely if pool is exhausted | Irregular, app-dependent | Soft | Must handle "0 available" gracefully |
| Return a buffer | Task finishes using it | N/A | Irregular | Soft | Must not double-return the same buffer |

### Step 2 — Task decomposition

*(Buffer-requesting tasks are existing application tasks — the design content here is the pool mechanism itself.)*

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Any requesting task | Take a buffer, use it, give it back | App-defined | App-defined | App-defined | Blocks on counting semaphore `take()` if pool is empty |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Requesting tasks ↔ buffer pool | "How many buffers are currently available" | **Counting semaphore**, initialized to 4 | This is a resource-counting problem (0 to 4 available), not a binary lock — a binary semaphore would only allow 1 buffer in use at a time, wasting the other 3 |
| Requesting tasks ↔ buffer pool | "Which specific buffer index is free" | **Separate mutex-protected free-list**, paired with the semaphore | The semaphore only tracks *count*, not *identity* — a second, small protected structure is needed to actually hand out a specific buffer index |

### Step 4 — Task graph

```
[Task X] --(take, count-- )--\
[Task Y] --(take, count-- )----> [Counting semaphore, init=4] --(paired with)--> [Mutex-protected free list: which index?]
[Task Z] --(give, count++)--/
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If a task holds a buffer far longer than expected, the pool's effective availability drops — with only 4 buffers, 2-3 slow holders can starve everyone else. Worth deciding: should buffer holds have a maximum duration or timeout enforced?
2. **Two events within 1ms?** Two tasks requesting simultaneously when only 1 buffer remains — the semaphore correctly serializes this (one gets it, one blocks), but confirm the free-list mutex prevents both from reading the same "free" index before either claims it.
3. **Cycle?** No, this is a resource pool pattern, not a producer/consumer chain.
4. **Starvation consequence?** A task requesting a buffer when all 4 are in use blocks until one is returned — acceptable if bounded; add a timeout if indefinite blocking isn't acceptable for that particular caller.
5. **Boot-time gap?** Semaphore must be initialized to 4 (matching actual buffer count) before any task starts — an off-by-one here (e.g. initializing to 5) would let a 5th "phantom" request succeed and corrupt memory.

---