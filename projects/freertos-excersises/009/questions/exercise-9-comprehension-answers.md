# Exercise 9 — Comprehension Check: Answer Key

Companion to `exercise-9-comprehension-questions.md`. Try answering first.

---

## Group A — Understanding the core problem

**1. Why is this resource-counting, not mutual exclusion?**

Mutual exclusion is a special case of resource counting where the resource count happens to be exactly 1 — "only one task may hold this at a time." Here there are genuinely 4 interchangeable units available simultaneously; up to 4 different tasks should legitimately be allowed to each hold one at the same time with no conflict at all between them. A binary semaphore/mutex hard-codes the assumption "count = 1," which is simply the wrong model for "4 of a thing exist and any number up to 4 can be in use concurrently." The counting semaphore generalizes that same take/give mechanism to any starting count, which matches the actual resource shape here.

**2. What would actually happen with a binary semaphore instead?**

The moment the first task takes the binary semaphore to claim a buffer, the semaphore is now "taken" (count effectively 0/1) — any other task trying to claim a *different, still-free* buffer would also block on the same semaphore, even though 3 more buffers are sitting completely unused. The system would behave as if there were only 1 buffer total, not 4 — you'd get correctness (no double-claim) but at the cost of throwing away 75% of your actual capacity for no reason. The bug isn't unsafe behavior, it's needless underutilization caused by using a primitive whose built-in assumption doesn't match the resource.

## Group B — Why two mechanisms, not one

**3. Division of labor between semaphore and mutex?**

The counting semaphore answers "how many buffers are currently available" — a pure count, updated atomically on every take/give, and it's the mechanism that makes a requesting task block automatically when the count hits zero. The free-list mutex answers a completely different question: "given that at least one is available, which specific buffer index is it?" The semaphore has no concept of individual buffer identity at all — it's just a number. One mechanism can't answer both because they're different data shapes: a scalar count vs. an array/bitmap of per-buffer state. Trying to force one primitive to do both jobs would mean either losing the atomic count tracking or losing the ability to identify a specific free buffer.

**4. Race condition if free-list has no mutex, even with a correct semaphore?**

Two tasks, X and Y, both successfully `take()` the semaphore (say 2 buffers are free, so both succeed, count goes from 2 to 0). Both proceed to scan `free_list` for a free index. Without a mutex around this scan, both could read the list at effectively the same moment, before either has written anything back — both see, say, index 2 as the first free buffer (because neither has marked it busy yet). Both then independently write "index 2 = busy" and both start using buffer 2 — meanwhile the *other* actually-free buffer (say index 3) never gets claimed by anyone and sits unused, while two tasks silently corrupt each other's data by writing to the same physical buffer. The semaphore correctly tracked *that* 2 buffers were available; it did nothing to ensure the *identification* of which 2 was done safely.

**5. Correct order: semaphore first or free-list lookup first?**

Semaphore `take()` must happen first. Reasoning: `take()` succeeding is the only guarantee that at least one buffer is actually free *right now* — attempting the free-list lookup first, without that guarantee, could find "no free buffer" when one exists (a stale read) or, worse, proceed as if one exists when the count is actually zero. Doing `take()` first converts "is a buffer free" into a real, atomically-checked precondition before you even attempt to identify which one — the free-list lookup then only has to solve the narrower problem of *which* one, never *whether* one exists, because the semaphore already settled that question.

## Group C — Break it on paper (from the exercise)

**6. Mechanism by which one slow task degrades unrelated tasks?**

With only 4 buffers total, each one held longer than necessary directly reduces the pool's effective size for everyone else — if 1 task holds a buffer 10x longer than typical, it's functionally removing 1 of the 4 slots from the shared pool for that entire duration. With only 4 total, losing even 1-2 slots to slow holders is proportionally huge (25-50% of total capacity gone), unlike a pool of, say, 1000 resources where one slow holder is statistically irrelevant. Other tasks needing a buffer during that window either block longer than expected or, if using a timeout, start failing/retrying — and crucially, those other tasks have no idea *why* they're being delayed, since they have no relationship to the slow task at all. This is structurally the same shape of problem as priority inversion (Exercise 6): an unrelated party's behavior degrades a task that has no direct dependency on it, just expressed through resource scarcity instead of scheduling priority.

**7. What goes wrong if the mutex covers only the read, not the write?**

This recreates almost exactly the race condition from Question 4, just with the mutex present but scoped incorrectly. Task X locks the mutex, reads free_list, sees index 2 free, **unlocks the mutex**, then (outside the lock) writes "index 2 = busy." Between the unlock and the write, Task Y can lock the mutex, read the still-unmarked free_list, also see index 2 as free, unlock, and also write "index 2 = busy." Both tasks now believe they exclusively hold buffer 2. The mutex protecting only the read accomplishes nothing for correctness — the entire point of the mutex is to make "read the state, decide, write the new state" a single indivisible unit; splitting the lock around only part of that sequence reopens the exact gap the mutex exists to close.

**8. How does a wrong semaphore count (init'd to 5) become memory corruption, not just a logic error?**

If the semaphore is initialized to 5 but the actual buffer array only has 4 valid entries (indices 0-3), then a 5th concurrent `take()` will succeed (the semaphore has no idea what the "real" buffer count is — it only knows the number it was told at init). That 5th task then goes to the free-list lookup expecting to find and claim a valid buffer — but there is no legitimate 5th buffer. Depending on the implementation, this typically means either the free-list logic returns/derefs an out-of-bounds index into the buffer array (reading/writing memory adjacent to the array — genuinely corrupting whatever data happens to live there, e.g. another variable or another task's data) or, if the free-list itself is sized to only 4, the 5th task gets stuck with an invalid state the code never anticipated. Either way, the failure has moved from "a number was wrong" to "a piece of memory outside the intended buffer array is being read or written" — which is the literal definition of memory corruption, and the kind of bug that manifests unpredictably somewhere unrelated later, not at the point where the actual mistake was made.

## Group D — Extending the scenario

**9. State of a 5th task blocked on `take()` with no timeout?**

The task is in the **Blocked** state (FreeRTOS terminology) — it consumes zero CPU time while waiting; the scheduler simply does not consider it for execution at all until something wakes it. It is woken specifically by a `give()` call on that exact semaphore from any other task (when a buffer is returned), at which point the RTOS moves it back to Ready, and it competes for the CPU normally from there based on its priority. This is fundamentally different from a polling loop, which would burn CPU cycles repeatedly checking — blocking on a semaphore costs nothing while genuinely idle.

**10. What must the code do differently on timeout vs. success, and the common mistake?**

On success, the code proceeds assuming it now legitimately owns a buffer (decremented the semaphore, has a valid index from the free-list). On timeout, the semaphore's count was **never decremented** — no buffer was actually claimed — so the code must take a completely different path: report failure/unavailability to whatever needed the buffer, and critically, must **not** proceed to touch the free-list or assume it holds any buffer at all. The most common mistake is code that doesn't check the return value of `take()` properly and falls through to "use buffer" logic regardless of whether the timeout occurred — effectively using an uninitialized or nonexistent buffer reference, which is a direct path to the same kind of memory corruption discussed in Question 8, just triggered by a logic bug instead of an off-by-one.

**11. Fundamental difference vs. Exercise 8's event group?**

Exercise 8 waits for **multiple distinct, named conditions to ALL become true** (bit 0 AND bit 1 AND bit 2) — the wait is about a specific combination of specific signals, and it doesn't matter how many are already true short of all three; partial progress (2 of 3) isn't useful on its own. Exercise 9 waits for **at least one unit of an interchangeable, fungible resource to become available** — it doesn't care *which* specific buffer, only that *some* count above zero exists; any one of the 4 is equally acceptable. Event groups model AND-combinations of distinct named events; counting semaphores model quantity of an undifferentiated pool. That's why one uses bit-flags with identity (which sensor) and the other uses a plain integer (how many buffers) — the underlying question each is answering has a genuinely different shape.

## Group E — Applying it beyond the exercise

**12. Removing the semaphore, just scanning free_list inside a mutex — would it work?**

Functionally, yes, this can be made correct: hold the mutex, scan for a free entry, mark it busy (or determine none exist), release the mutex — no double-claim race, assuming the scan-and-mark is fully inside the lock. What's lost: the automatic blocking behavior. With the semaphore, a task calling `take()` on an empty pool is automatically put to sleep by the RTOS and woken precisely when a buffer is returned — zero CPU cost while waiting. Without the semaphore, "no buffer free" inside the mutex-protected scan just means the function returns a failure/empty result immediately (mutexes don't provide a "wait until a condition changes" mechanism on their own) — the caller would need to implement their own retry/poll loop, or their own additional blocking mechanism, to get equivalent wait-for-availability behavior. You'd be reimplementing, poorly, exactly what the semaphore already provides for free.

**13. What to flag about a hardcoded `= 4` init separate from the buffer array's size?**

Flag it as a maintainability/correctness hazard waiting to happen: the semaphore count and the actual array size are two independent numbers that must always agree, but nothing in the code enforces that they do — exactly the off-by-one scenario from Question 8. If someone later resizes the buffer array (say, to 6 buffers) and forgets the separate hardcoded semaphore initialization, the system silently reintroduces the exact out-of-bounds/memory-corruption bug discussed earlier, with no compiler warning and no obvious connection between the two edited lines. The fix to request: initialize the semaphore using a size constant/macro shared with (or derived from) the buffer array's actual declared size, so the two values are structurally tied together and can't drift apart silently.
