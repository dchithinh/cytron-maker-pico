
## Exercise 6 — Engineer priority inversion, then fix it

**System name:** Priority inversion demonstrator
**One-sentence description:** Three tasks of different priority share a resource in a way that lets a low-priority task indirectly block a high-priority one for far longer than intended.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| High-priority work | Timer/event | Short, tight | Frequent | Hard (for the exercise's sake) | Must not be blocked by lower-priority activity |
| Medium-priority work | Timer/event | Moderate | Frequent, CPU-heavy | Soft | Unrelated to the shared resource |
| Low-priority work | Timer/event | Loose | Infrequent | Soft | Holds the shared resource briefly while doing its work |

### Step 2 — Task decomposition

| Task name   | Responsibility                                        | Priority | Priority justification                          | Stack estimate | Blocking behavior                     |
| ----------- | ----------------------------------------------------- | -------- | ----------------------------------------------- | -------------- | ------------------------------------- |
| High task   | Needs the shared resource briefly                     | High     | Tightest deadline                               | 512B           | Blocks on shared-resource lock        |
| Medium task | Unrelated CPU-bound work, no resource interaction     | Medium   | Own deadline, doesn't touch the shared resource | 512B           | Rarely blocks; mostly runs when ready |
| Low task    | Acquires shared resource, does some work, releases it | Low      | Loosest deadline                                | 512B           | Holds lock during its work window     |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why (and the bug) |
|---|---|---|---|
| Low task ↔ High task | Exclusive access to shared resource | **Raw binary semaphore used as a lock (the bug)** | Without priority inheritance, Low can be preempted by Medium repeatedly while holding the lock, and High — despite being highest priority — is stuck waiting on Low, which itself can't run. High is effectively blocked by Medium, a lower-priority task. |
| **Fix:** Low task ↔ High task | Same | **Mutex with priority inheritance** | When High blocks on the mutex, Low's priority is temporarily boosted to High's level, so Medium can no longer preempt Low. Low finishes quickly, releases, High unblocks. |

### Step 4 — Task graph

```
Broken version:
[Medium task] --(preempts, unrelated)--> [Low task] --(holds)--> [shared resource] <--(waits)-- [High task]
                                                                     (raw semaphore, no inheritance)

Fixed version:
[Low task] --(holds, priority temporarily boosted)--> [shared resource, MUTEX] <--(waits)-- [High task]
[Medium task] can no longer preempt Low while it holds the boosted-priority mutex
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** In the broken version, if Medium's workload doubles, High's effective wait time doubles too — this is the core danger of unbounded priority inversion: the highest-priority task's latency becomes hostage to an unrelated, lower-priority task's load.
2. **Two events within 1ms?** Not directly relevant here — the hazard is about lock hold duration and preemption, not event timing.
3. **Cycle?** No structural cycle, but there's an *effective* cycle in the broken version: High waits on Low, Low's progress depends on not being preempted, and Medium's unrelated activity controls that — a three-way dependency that isn't a deadlock but produces the same practical symptom (High stalls far longer than its priority should allow).
4. **Starvation consequence?** This is exactly what the exercise demonstrates: High, despite being top priority, is starved by a chain of lower-priority activity. The fix removes this by ensuring Low can't be preempted by Medium while blocking High.
5. **Boot-time gap?** N/A for this exercise.

**Reference:** this is precisely the mechanism behind the Mars Pathfinder priority inversion incident — worth reading the real postmortem once you've built this scenario, since you'll recognize every part of it.

---