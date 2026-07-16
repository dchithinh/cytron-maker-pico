
Exercise 11 — Engineer a deadlock, then fix it with lock ordering

**System name:** Circular-wait deadlock demonstrator
**One-sentence description:** Two tasks each need two shared mutexes but acquire them in opposite order, creating a classic circular-wait deadlock.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Task 1's combined operation | Event | Moderate | Irregular | Soft (for the exercise) | Needs Mutex A then Mutex B |
| Task 2's combined operation | Event | Moderate | Irregular | Soft | Needs Mutex B then Mutex A (the bug) |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Task 1 | Acquire A, then B, do work, release both | Medium | App-defined | 512B | Can block indefinitely on B if Task 2 holds it (the bug) |
| Task 2 | Acquire B, then A, do work, release both | Medium | App-defined | 512B | Can block indefinitely on A if Task 1 holds it (the bug) |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why (and the bug) |
|---|---|---|---|
| Task 1 ↔ shared resources | Exclusive access to A, then B | Mutex A, Mutex B, **acquired in order A→B (the bug: inconsistent with Task 2)** | If Task 1 holds A and waits for B, while Task 2 holds B and waits for A, both wait forever — all four deadlock conditions present (mutual exclusion, hold-and-wait, no preemption, circular wait) |
| **Fix:** Task 1 ↔ shared resources | Same | Mutex A, Mutex B, **both tasks now acquire in the same global order (A before B, always)** | Eliminates circular wait by construction — Task 2 is rewritten to acquire A first even though its natural work order might suggest B first |

### Step 4 — Task graph

```
Broken version:
[Task 1] --holds--> [Mutex A] <--waits-- [Task 2]
[Task 2] --holds--> [Mutex B] <--waits-- [Task 1]
(circular wait -> deadlock)

Fixed version:
[Task 1] --acquires A, then B--> both released --> done
[Task 2] --acquires A, then B (rewritten order)--> both released --> done
(no cycle possible; whichever task gets A first proceeds, the other simply waits its turn for A, then B)
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** In the fixed version, if Task 1 holds both mutexes for longer than expected, Task 2 simply waits longer — no deadlock risk, just a serialization delay. This is the expected, safe behavior of the fix.
2. **Two events within 1ms?** Not the relevant hazard — this exercise is about acquisition order, not event timing.
3. **Cycle in the graph?** Yes, explicitly, in the broken version — that's the entire point of the exercise. The fixed version has no cycle by construction.
4. **Starvation consequence?** In the broken version, both tasks are permanently stuck — worse than starvation, this is total deadlock with zero forward progress for either task.
5. **Boot-time gap?** N/A for this exercise.

**Professional habit this builds:** document lock-ordering rules explicitly (e.g., a comment at each mutex's definition stating its position in the global order) so a future engineer adding a third mutex doesn't accidentally reintroduce a cycle. An alternative fix — a single combined mutex protecting both resources together — is simpler but serializes access more than necessary; lock ordering is usually preferred because it's a static, inspectable rule rather than a runtime behavior you have to test for.