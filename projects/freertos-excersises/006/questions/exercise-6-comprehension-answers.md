# Exercise 6 — Comprehension Check: Answer Key

Companion to `exercise-6-comprehension-questions.md`. Try answering first — reading this before attempting defeats the point.

---

## Group A — Understanding the core problem

**1. Why is Medium the troublemaker, even though it never touches the shared resource?**

Medium doesn't need to touch the resource to cause damage — it only needs to be able to **preempt Low** while Low is holding it. Because Medium has higher priority than Low, the scheduler will always run Medium over Low whenever Medium is ready, regardless of what Low is doing. So every time Medium becomes ready, Low gets suspended mid-critical-section, the resource stays locked, and High — who has nothing to do with Medium — is stuck waiting on Low, who is stuck waiting on Medium. Medium is an *innocent bystander* whose mere presence and priority level is enough to extend High's wait indefinitely. This is what makes classic priority inversion so counterintuitive: the task causing the delay isn't even in the dependency chain functionally, only schedule-wise.

**2. Without Medium, does inversion still happen?**

Yes, but it's **bounded and expected**, not the pathological case the exercise is demonstrating. With only High and Low: High blocks on the resource, waits for exactly as long as Low's critical section takes, then proceeds. This is normal, bounded priority inversion — every real-time system has *some* of this and accounts for it in schedulability analysis. What Medium adds is **unbounded** inversion — High's wait time is no longer tied to Low's critical section length, but to however long Medium (and any other unrelated medium-priority work) keeps running. That distinction — bounded vs. unbounded — is the actual crux of why this is a "bug" worth engineering a fix for.

**3. Why is a binary semaphore "the bug" even though it functionally works?**

It works in the sense that it correctly enforces mutual exclusion — only one task can hold it at a time. But a semaphore has no concept of **ownership**: the RTOS doesn't track "who is holding this and at what priority they're currently running." Because of that, the scheduler has no information it could use to intervene even if it wanted to — there's no hook for "temporarily treat Low as if it were High priority" because the semaphore API was never designed to carry that relationship. A mutex, in RTOS implementations that support priority inheritance, does track ownership, which is exactly the missing piece that makes the fix possible. So "works" here means "achieves mutual exclusion" — it does not mean "behaves safely under all scheduling conditions," and that gap is the bug.

## Group B — The fix (Priority Inheritance)

**4. What exactly changes when High starts waiting?**

Low's **effective priority** is temporarily raised to match High's priority — not Low's *base* priority (the one it was created with), but a second, temporary "current effective priority" the scheduler uses for scheduling decisions. The moment High calls `xSemaphoreTake()` (mutex variant) and blocks because Low holds it, the RTOS kernel detects this and boosts Low. Nothing changes on High itself — High just blocks normally, waiting.

**5. What happens after Low releases the mutex?**

Low's effective priority reverts back down to its original base priority immediately upon release. It does not stay boosted — if it did, Low would now be running at High's priority level for its *entire remaining life*, defeating the purpose of having priority levels in the first place (see Q6).

**6. What would go wrong if the boost were permanent?**

You'd get priority level inflation — every task that ever briefly held a contested mutex would permanently join the highest priority tier it was ever boosted to, collapsing the whole priority scheme over time. Low-priority background work would start starving other legitimately low-priority tasks, and the system would gradually lose the ability to express "this work matters less" at all. The boost has to be exactly as long as the blocking dependency requires — no shorter (or High still waits too long) and no longer (or you break the priority scheme).

**7. Does inheritance eliminate the wait, or just bound it?**

Just bounds it. High still has to wait for Low to finish its critical section — that part is unavoidable and correct (High genuinely can't have the resource until Low is done with it). What priority inheritance removes is the *extra*, unbounded wait caused by Medium (or other unrelated tasks) preempting Low during that window. So the guarantee becomes: High waits **at most** as long as Low's critical section takes, not "as long as Low's critical section takes, plus however long unrelated medium-priority work happens to run."

## Group C — Task graph & structural reasoning

**8. What makes the broken version feel like a cycle without being one structurally?**

A true cycle (as in deadlock, Exercise 11) requires each party to be *waiting on* the other, in a closed loop, with the loop being the entire cause of the stall. Here there's no such loop: Medium doesn't wait on anything related to High or Low, and Low doesn't literally wait on Medium either — Low simply *doesn't get scheduled* while Medium is runnable, which is a scheduling-priority effect, not a blocking dependency. But the practical symptom looks identical to a cycle: High → waiting on Low → whose progress is controlled by → Medium's behavior, and nothing in that chain moves forward on High's timescale until Medium stops. It's a three-way dependency chain producing cycle-like starvation without being a graph cycle in the formal sense — worth keeping distinct from real deadlock, since the fix (priority inheritance) is completely different from a deadlock fix (lock ordering).

**9. Why does Medium deliberately have zero interaction with the resource?**

If Medium also sometimes needed the resource, the scenario would conflate two different problems: ordinary contention (multiple tasks legitimately competing for the same resource, whose queuing is expected and bounded) and priority inversion (an unrelated task's mere existence extending an unrelated wait). By making Medium's interaction with the resource exactly zero, the exercise isolates the inversion effect cleanly — any delay to High that Medium causes can *only* be attributed to scheduling interference, not legitimate resource contention. This is a deliberate experimental-design choice, not an incidental detail.

## Group D — Break it on paper

**10. Why is the "double Medium's load → double High's wait" relationship dangerous for a hard real-time task?**

A hard real-time task's entire premise is that its deadline is a hard constraint — missing it isn't "slow," it's a system failure (a watchdog not fed in time triggers a reset; a safety interlock not evaluated in time means an unsafe condition goes unchecked). If High's wait time scales with Medium's unrelated workload, then High's worst-case response time is no longer determined by High's own design — it's hostage to whatever Medium happens to be doing, which may not even be under the same team's control or may change in a future firmware update. Concretely: a watchdog-feed task with this bug might work fine in testing, then start missing deadlines and triggering false resets months later when someone adds unrelated Medium-priority work elsewhere in the codebase, with no obvious connection between the two.

**11. Does inheritance still fully solve it with a second, unrelated medium-priority task?**

Yes — this is worth being precise about. Priority inheritance boosts Low to High's priority level, and at that level, **any** medium-priority task (there could be one, two, or ten) can no longer preempt Low, because Low is now running above all of them. The fix isn't "neutralize this one specific Medium task" — it's "make Low un-preemptable by anything below High's level for the duration it holds the resource." So the fix generalizes correctly regardless of how many unrelated medium-priority tasks exist in the system.

**12. What likely happened on Mars Pathfinder (before looking it up)?**

The general shape: a low-priority task held a mutex protecting a shared information bus. A higher-priority task (bus management) needed that same mutex and blocked waiting for it. Meanwhile, a medium-priority task — unrelated to the bus — ran for a long stretch, repeatedly preempting the low-priority task and preventing it from finishing and releasing the mutex. The high-priority task's wait grew far longer than intended, eventually triggering a watchdog timeout, which caused the system to reset. It's the textbook version of exactly this exercise, discovered in production on actual spacecraft hardware, which is part of why it's such a famous case study.

## Group E — Applying it beyond the exercise

**13. Would priority inversion still be a concern under `taskENTER_CRITICAL()`?**

No, not in the same form — and the reason is instructive. Priority inversion is fundamentally a *scheduling* phenomenon: it depends on other tasks (or interrupts) being able to run while one task holds a resource. `taskENTER_CRITICAL()` disables interrupts, which also halts the scheduler's ability to preempt at all — nothing else can run, period, until the critical section exits. With nothing else able to run, there's no Medium available to cause unbounded delay. The tradeoff, covered earlier, is that this protects against inversion by being far more blunt: it stalls the *entire system*, not just tasks contending for one resource — which is why it's reserved for very short sections rather than used as a general-purpose substitute for mutexes.

**14. How would you detect this bug in production, concretely?**

Use an RTOS-aware trace tool (e.g., SEGGER SystemView, Percepio Tracealyzer, or FreeRTOS's built-in trace hooks) to capture task state transitions over time and look for the specific signature: High enters the Blocked state waiting on the mutex, Low is in Ready-but-not-Running (preempted) for an extended stretch, and Medium shows extended Running periods during exactly that window. The visual giveaway is a gap between "High blocks" and "High unblocks" that doesn't correlate with Low's actual execution time, but does correlate with Medium's execution bursts during that same window. Simply logging timestamps around the mutex take/give on High alone wouldn't reveal *why* — you need visibility into what Low and Medium were doing during the gap, which is exactly what a scheduler trace gives you and ad-hoc printf logging generally doesn't.

**15. What would you ask a colleague using a binary semaphore across different-priority tasks?**

Something like: "Is there any medium-priority task in the system that could preempt whichever task is holding this semaphore? If so, how do we bound the resulting wait — do we need priority inheritance here, which means this should probably be a mutex instead of a raw semaphore?" The underlying question you're really checking is whether they've considered the *priority relationship* between the tasks sharing this resource, not just whether the locking mechanism prevents simultaneous access. A semaphore being technically correct for mutual exclusion is not the same question as whether it's the *right* primitive given the priority levels involved — and that's precisely the distinction this whole exercise is built to teach.
