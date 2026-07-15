# Exercise 7 — Comprehension Check: Answer Key

Companion to `exercise-7-comprehension-questions.md`. Try answering first.

---

## Group A — Understanding the core problem

**1. Why is overflow guaranteed, with the math?**

Producer generates 1 item every 10ms → 100 items/second. Consumer processes 1 item every 100-150ms → roughly 6.7 to 10 items/second. Even at the consumer's *best case* (100ms, 10 items/sec), the producer still outpaces it 10x. Over any sustained window — say 1 second — producer creates 100 items, consumer drains at most 10. The queue gains ~90 unconsumed items every second it runs. There's no steady state where the queue stabilizes; it grows without bound until something (the queue's fixed size) forces items to be dropped or the producer to block. This isn't a probability, it's arithmetic — overflow isn't a risk to mitigate, it's a certainty to plan for.

**2. "Structural" vs. "occasional" — why the distinction matters?**

An occasional slowdown implies the system is correctly sized for normal conditions and only misbehaves in rare, extreme cases — the right response is usually "handle the edge case gracefully and move on." A structural mismatch means the system is **never** correctly sized — overflow isn't the exception, it's the default behavior under any sustained operation. This changes what "handling it well" even means: you're not writing an edge-case handler, you're designing the *primary* behavior of the system, because the "overflow case" IS the normal case here.

## Group B — Choosing an overflow policy

**3. What property of the data decides the policy?**

Whether an old, unconsumed item is still *useful* once a newer one exists. If data represents a snapshot of current state (temperature reading, position, sensor value) — the newest value supersedes the old one in usefulness, so dropping old data loses nothing meaningful (drop-oldest). If data represents discrete events or commands that each need to happen (a button press, a command to move a motor by X, a log entry) — every item carries unique information that can't be reconstructed from a later item, so dropping any of them loses real information (drop-newest, or block).

**4. Example where drop-oldest would be dangerous?**

A queue of movement commands for a robotic arm (e.g., "move joint 3 by +5°," "move joint 3 by +5°," "move joint 3 by -3°"). These are *relative*, sequential instructions — dropping an old one doesn't just lose a stale snapshot, it corrupts the arm's actual trajectory, because each command depends on the physical result of the ones before it actually executing. Unlike a sensor reading, there's no "latest value" that makes the older ones irrelevant — they're cumulative, not supersede-able.

**5. Block-the-producer and the 10ms guarantee?**

If the producer blocks waiting for queue space, it can no longer reliably run every 10ms — its timing becomes dependent on however long the consumer takes to free up space, which per the requirements is 100-150ms. This directly violates the producer's own stated period. Looking back at Step 1's requirements table: the producer's deadline is marked N/A / soft, so this *might* be tolerable — but only if nothing else in the system actually depends on the producer's 10ms cadence being real. If anything upstream assumes the 10ms period holds, block-the-producer silently breaks that assumption. It's a valid choice only if you've explicitly confirmed the producer's timing truly doesn't matter to anything else.

## Group C — The deeper point of the exercise

**6. Cosmetic vs. structural fix?**

A cosmetic fix changes what happens *after* overflow occurs — which items get kept or discarded — without changing the underlying rate mismatch that causes overflow in the first place. The system still can't keep up; you've just chosen how it fails gracefully. A structural fix changes the actual throughput relationship between producer and consumer, so that overflow stops being inevitable. Choosing drop-oldest vs. drop-newest answers "how do we fail," not "how do we stop failing" — and the exercise is explicitly pointing out that Step 3's policy table, however carefully reasoned, is still only answering the first question.

**7. Two real architectural fixes, with tradeoffs?**

- **Slow the producer** (e.g., sample every 100ms instead of 10ms): directly closes the rate gap. Tradeoff: you lose whatever value the higher sample rate provided — if the extra samples mattered (e.g., catching transient spikes), you lose real information, not just queue pressure.
- **Batch processing on the consumer side** (consumer pulls and processes multiple queued items per cycle instead of one): increases effective consumer throughput without touching the producer. Tradeoff: added complexity in the consumer's logic, and it only helps if the consumer's 100-150ms cost is partly fixed overhead (e.g., a flash write's setup cost) rather than fully proportional to one item — if it's proportional, batching doesn't actually speed up the per-item rate.

**8. When does raising consumer priority help vs. not?**

It helps only if the consumer's slowness is caused by **starvation** — i.e., the consumer is ready to run but a higher-or-equal-priority task keeps preempting it, artificially inflating its 100-150ms figure beyond its true processing cost. In that case, raising its priority lets it actually get CPU time and the real per-item time drops. It does nothing (or makes things worse) if the 100-150ms is the consumer's genuine, unavoidable processing cost (e.g., an actual flash write taking that long) — no amount of priority fixes a task that's CPU-ready but physically waiting on hardware I/O, and raising its priority in that case just risks starving *other* tasks in the system for no benefit.

## Group D — Break it on paper, extended

**9. Concrete scenario where block-the-producer is genuinely bad?**

A producer sampling a fast-changing analog sensor — say, current draw during a motor's startup transient — needs to capture points at fixed 10ms intervals to reconstruct the transient shape accurately for later analysis or a safety check. If the producer blocks waiting for queue space, it misses sampling *during the actual event it was designed to observe*, and by the time it unblocks, the transient is over. The data most needed is exactly the data lost — worse than dropping old queued values, because the real-world event that mattered doesn't wait for the software to catch up.

**10. Overlap with Exercise 6 if the 150ms spike comes from CPU contention?**

Yes — this reframes the queue overflow problem as potentially being a **symptom** of priority inversion rather than a genuine processing-cost issue. If the consumer's 150ms spike happens specifically when a lower-priority task hogs the CPU due to an unrelated medium-priority task interfering (the Exercise 6 scenario), then the "fix" isn't an overflow policy or even batching — it's diagnosing and fixing the inversion itself, after which the consumer's real processing time might turn out to be much closer to 100ms consistently. This is exactly the kind of connection worth checking in practice: before designing an elaborate overflow policy, confirm the consumer's slowness is real work, not an artifact of a scheduling bug elsewhere.

**11. What to measure in production to confirm the policy is working?**

Queue high-water mark / actual occupancy over time (most RTOS queue implementations expose current or peak usage), the actual drop rate under real load (how often the overflow policy is triggered, not just whether it *can* trigger), and the consumer's real measured per-item processing time distribution (not just the 100-150ms assumed on paper) to catch drift or unexpected spikes. If the drop rate in production is far higher than expected, or the consumer's measured timing doesn't match the assumed 100-150ms, the design assumptions from Step 1 need revisiting — paper analysis is a starting hypothesis, not a guarantee.

## Group E — Applying it beyond the exercise

**12. Why doesn't "just make the queue bigger" fix it?**

At a 10-15x sustained rate mismatch, a bigger queue only delays the moment overflow starts — it doesn't change the fact that the queue's occupancy grows without bound under sustained load. Do the math: even a queue 10x larger only buys roughly 10x longer before it fills, assuming the mismatch persists (which, per the requirements, it does — this isn't a burst, it's the steady-state rate). If the system runs for any extended period, the bigger queue still fills, just later — and now you've also added more RAM usage and (if it does fill) a larger backlog of stale data to drop all at once. It converts an immediate problem into a delayed, harder-to-notice one, without solving it.

**13. What to flag in a PR with no explicit overflow policy?**

Flag that the queue is implicitly relying on the RTOS default overflow behavior (commonly: reject the new item and return a failure/timeout to the producer, or block the producer depending on the API used) — and that whatever that default is, it was not a deliberate choice tied to what the data represents. The core point: "we didn't think about it" doesn't mean "there is no policy" — the default behavior of the underlying queue API *is* the policy by omission, and it may be exactly wrong for this data (e.g., silently blocking a producer that was assumed to be non-blocking). The question to ask: "What happens to data when this queue is full, and was that the deliberate right choice for what this data represents — or just whatever the API defaults to?"
