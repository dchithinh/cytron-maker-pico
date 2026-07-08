
## Exercise 4 — Shared UART from two tasks

**System name:** Shared debug UART
**One-sentence description:** Two independent tasks both need to print to the same UART without corrupting each other's output.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Print debug message (Task A) | Application event | None (best-effort) | Irregular | Soft | Must not corrupt interleaved output |
| Print debug message (Task B) | Application event | None (best-effort) | Irregular | Soft | Same UART resource |

### Step 2 — Task decomposition

*(Tasks A and B are pre-existing application tasks — this exercise is purely about the shared resource, not new task creation.)*

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Task A | Existing app logic + occasional UART print | (App-defined) | (App-defined) | (App-defined) | Blocks briefly on UART mutex when printing |
| Task B | Existing app logic + occasional UART print | (App-defined) | (App-defined) | (App-defined) | Blocks briefly on UART mutex when printing |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Task A ↔ Task B (via shared resource) | Exclusive access to UART hardware | **Mutex** (not binary semaphore) | This is mutual exclusion of a resource, not event signaling — a real mutex also gets priority inheritance for free, protecting against inversion if a low-priority task holds it while a high-priority task wants to print |

### Step 4 — Task graph

```
[Task A] --\
            \--> [UART, protected by mutex]
[Task B] --/
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If Task A holds the mutex for an unusually long print (e.g. a large debug dump), Task B blocks for that whole duration. Acceptable if prints are short; if not, consider a bounded print length or moving large dumps through a queue instead.
2. **Two events within 1ms?** Both tasks could want to print near-simultaneously — the mutex serializes them correctly by design; no data corruption, just ordering (acceptable for debug output).
3. **Cycle?** No — this is a shared-resource pattern, not a producer/consumer chain.
4. **Starvation consequence?** If a high-priority task needs the mutex while a low-priority task holds it, priority inheritance (built into a real mutex) prevents the inversion scenario explored in Exercise 6.
5. **Boot-time gap?** N/A — mutex starts unlocked, first `take()` succeeds immediately.

**Gotcha called out in this exercise:** if an ISR also needs to log something, it *cannot* take this mutex — ISRs must never block. Real designs route ISR-originated log messages through a separate lock-free ring buffer or queue drained by a dedicated logging task, never sharing the UART mutex path between ISR and task contexts.

---