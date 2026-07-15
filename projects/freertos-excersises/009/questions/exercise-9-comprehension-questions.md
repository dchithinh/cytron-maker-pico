# Exercise 9 — Comprehension Check Questions

Companion questions for "Counting semaphore for a buffer pool." Answer in your own words before checking the answer key.

---

## Group A — Understanding the core problem

1. Why is this problem framed as a **resource-counting** problem rather than a mutual-exclusion problem? What specifically about "4 fixed buffers" makes a binary semaphore the wrong tool?

2. The answer key says a binary semaphore "would only allow 1 buffer in use at a time, wasting the other 3." Explain precisely why — what would actually happen if you tried to use a binary semaphore here instead of counting?

## Group B — Why two mechanisms, not one

3. Explain the division of labor between the counting semaphore and the free-list mutex. What question does each one answer, and why can't one mechanism answer both?

4. Construct the race condition that would occur if the free-list were **not** protected by a mutex, even though the counting semaphore is working correctly. Walk through it step by step with two tasks.

5. What is the correct *order* of operations when a task requests a buffer — semaphore `take()` first or free-list lookup first? Justify why the order matters, not just what the order is.

## Group C — Break it on paper (from the exercise itself)

6. Step 5 raises the question of whether buffer holds should have a maximum duration or timeout. Explain the mechanism by which a single slow task can degrade the experience for tasks that have nothing to do with it, using the "only 4 buffers" constraint specifically.

7. Step 5 point 2 says to "confirm the free-list mutex prevents both from reading the same free index before either claims it." Describe concretely what would go wrong if the mutex were held only around the *read* of the free-list, but released before the *write* (marking the chosen index as busy).

8. Step 5 point 5 flags an off-by-one initialization bug (initializing the semaphore to 5 instead of 4) as capable of "corrupting memory." Explain the mechanism — how does a wrong semaphore count turn into actual memory corruption, not just a logic error?

## Group D — Extending the scenario

9. Suppose a 5th task calls `take()` when all 4 buffers are in use, with no timeout specified. What is this task's state from the scheduler's point of view while it waits — is it consuming CPU time, and what wakes it up?

10. Now suppose that same 5th task's `take()` call has a timeout, and the timeout fires. What must the calling code do differently compared to the case where `take()` succeeds — what's the most common mistake here?

11. Compare this exercise's synchronization design to Exercise 8's event group. Both involve waiting for something to become available/ready — what's the fundamental difference in *what* they're waiting for that explains why one uses a counting semaphore and the other uses an event group?

## Group E — Applying it beyond the exercise

12. A colleague suggests simplifying the design by removing the counting semaphore entirely and just having tasks check `free_list` directly (loop through it, find a free entry, mark it busy — all inside the mutex). Would this work correctly? What would be lost, if anything, compared to the semaphore + mutex combination?

13. If you were reviewing a PR for this system and saw the counting semaphore initialized as a **global variable with `= 4` hardcoded**, separately from wherever the actual buffer array is declared with its own size constant, what would you flag?

---

## How to use this

- Group A and B check whether you understand *why* two separate mechanisms are needed, not just that the answer key uses two.
- Group C works directly from the "break it on paper" section — these are the exact kinds of failure modes a reviewer or interviewer would probe.
- Group D and E test transfer — connecting this exercise to Exercise 8, and reasoning about a design variant not explicitly covered.
