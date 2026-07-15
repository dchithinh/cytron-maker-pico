# Exercise 6 — Comprehension Check Questions

Companion questions to test understanding of "Engineer priority inversion, then fix it" beyond just remembering the answer key. Answer in your own words, without looking at the answer key.

---

## Group A — Understanding the core problem (Step 1–3)

1. Why is the **Medium task** the troublemaker in this scenario, even though it never touches the shared resource that High needs? Explain the exact mechanism.

2. If Medium task were removed from the system (only High and Low remain), would Low holding the resource still cause priority inversion? Why or why not?

3. In Step 3, the answer key labels "raw binary semaphore used as a lock" as **the bug**. Why is using a binary semaphore as a lock a problem, given that it still "works" functionally (it still locks/unlocks)?

## Group B — The fix (Priority Inheritance)

4. Explain precisely what happens to priority when High starts waiting on the mutex that Low is holding — what number changes, and on which task?

5. After Low releases the mutex, what happens to Low's priority? Does it stay boosted, or revert — and why does that matter?

6. Priority inheritance is described as "temporary." What would go wrong if the priority boost were permanent instead?

7. Does priority inheritance eliminate High's wait entirely, or does it just bound/limit it? What is High still guaranteed to wait for, even with the fix in place?

## Group C — Task graph & structural reasoning

8. In the "broken version" task graph, the answer key describes an "effective cycle" that isn't a real structural cycle. Explain in your own words what makes it *feel* like a cycle even though it technically isn't one (this connects to deadlock reasoning in Exercise 11 — worth comparing).

9. Why does the exercise deliberately give Medium task **no interaction at all** with the shared resource? What would be lost from the demonstration if Medium also needed the resource sometimes?

## Group D — "Break it on paper" — pushing the scenario further

10. The answer key says: if Medium's workload doubles, High's wait time roughly doubles too. Why is this relationship dangerous specifically for a **hard real-time** task like High? What does it mean concretely if High is a "watchdog-feed" or "safety interlock" task instead of a generic example?

11. Suppose there were a **second** medium-priority task, also unrelated to the shared resource, also capable of preempting Low. Does priority inheritance (boosting Low to High's level) still fully solve the problem? Why or why not?

12. The answer key references the real Mars Pathfinder incident. Based on what you now understand about this exercise, describe in your own words what likely happened on Pathfinder, without looking it up first.

## Group E — Applying it beyond the exercise

13. Suppose instead of a mutex, the shared resource were protected by `taskENTER_CRITICAL()` around the same critical section. Would priority inversion still be a concern? Why or why not — connect this to what "priority" even means once interrupts are disabled.

14. In a real system, how would you *detect* that this exact bug is happening in production — before you know a fix is needed? Name a concrete tool or technique, not just "I would notice High is slow."

15. If you were reviewing a colleague's PR and saw a binary semaphore being used to protect a shared resource between tasks of different priorities, what specific question would you ask them before approving it?

---

## How to use this

- Try to answer all of Group A and B fully in your own words before checking anything.
- Group C and D are where genuine understanding (vs. memorized answer) shows up — if you can only restate the answer key's wording, that's a sign to go back and rebuild the reasoning from scratch.
- Group E tests whether you can transfer the concept to situations not explicitly covered in the exercise — this is closest to what an interview question would actually look like.
