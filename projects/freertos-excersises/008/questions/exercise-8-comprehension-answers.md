# Exercise 8 — Comprehension Check: Answer Key

Companion to `exercise-8-comprehension-questions.md`. Try answering first.

---

## Group A — Understanding the core problem

**1. Why an AND-wait on 3 bits, rather than reading three shared variables in sequence?**

If Fusion just read three shared variables directly (A, then B, then C) without any synchronization signal, it has no way to know whether each sensor has actually *finished writing* its value yet — it might read a stale value from the previous cycle, or a value that's only half-written (a torn read), because nothing is telling Fusion "this data is ready now." The event group isn't there to move the data — it's there to answer the separate question "has each producer signaled completion," which the raw variables alone can't tell you. Reading the variables is only safe *after* you know, via the event bits, that each write is done.

**2. Why does "any order" completion rule out three sequential semaphore waits?**

`take(A)` then `take(B)` then `take(C)` in that order forces Fusion to wait for A first, no matter what. If C actually finishes first and A is slow, Fusion is sitting blocked on `take(A)` while a `give(C)` has already happened and is just sitting there unused — Fusion can't "see" that C is ready until it works its way through A and B first. The sequential wait imposes an artificial ordering constraint that doesn't match reality: the sensors don't know or care about a required completion order, but the code would be behaving as if they did. An event group has no notion of order at all — it just tracks which bits are set, and the AND-wait naturally unblocks the instant the last of the three (whichever one that happens to be) sets its bit.

## Group B — Event groups vs. other mechanisms

**3. Would a counting semaphore initialized to 3 work as a substitute?**

Partially — a counting semaphore can tell you *how many* signals have arrived (e.g., "give" 3 times, then a `take` sequence of 3 succeeds), which superficially resembles "wait for all three." But it can't tell you **which** three arrived. If Sensor A signals twice due to a bug and Sensor C never signals at all, the semaphore's count still reaches the same value (2 + 1 from B = 3, purely by coincidence) and Fusion would proceed thinking all three are ready when C never actually reported. The event group avoids this because each sensor owns a *specific, distinct bit* — the AND-wait condition is only satisfied when bits 0, 1, AND 2 are individually set, not when some total count is reached regardless of source. This distinction — identity-tracking vs. count-tracking — is exactly the information the semaphore throws away.

**4. Why no data payload, and where does the actual sensor data go?**

An event group's bits are just boolean flags (set/clear) — the API has no mechanism to attach a value to a bit, only a "this happened" signal. The actual sensor readings need a separate channel: typically shared variables (protected by a mutex, or made safe via atomic access if small enough) or per-sensor queues that Fusion reads from once it knows, via the event bits, that it's safe to do so. The design deliberately separates **"is data ready"** (event group's job) from **"what is the data"** (a different mechanism's job) — trying to cram both into one primitive would mean picking a mechanism that's mediocre at both jobs instead of good at either.

## Group C — The actual gotcha of the exercise

**5. State the gotcha in one sentence.**

If any one sensor never sets its bit (e.g., due to hardware failure), the Fusion task — if waiting without a timeout — blocks forever, even though the other two sensors are working fine and have data ready to use.

**6. Why is "wait for all three" riskier than "wait for one"?**

Waiting on N independent things to all succeed means the *system's* probability of failure is the sum of the individual failure probabilities (roughly, for small probabilities) — every one of the three sensors becomes a single point of failure for the Fusion task, even though Fusion doesn't care about any one of them specifically, it needs all of them. With one sensor, only that sensor's failure blocks Fusion. With three, Fusion's task is now hostage to whichever of the three fails first — the AND condition means the system's reliability is bounded by its *weakest* link, not by the average of the three.

**7. Why is a timeout alone still incomplete without a fallback?**

A timeout only guarantees that the blocking `wait` call *returns* — it doesn't specify what the Fusion task should actually *do* the moment that happens. Without an explicit fallback coded, the most likely outcomes are either: the code proceeds to use whichever data variables exist, silently treating incomplete/stale data as if it were a valid full reading (wrong answer, no warning), or the code has no branch for the timeout case at all and hits undefined/unhandled behavior. A timeout converts "block forever" into "eventually stop waiting" — but "then what" is a separate design decision that has to be made explicitly, not something the timeout mechanism decides for you.

## Group D — Break it on paper, extended

**8. New failure mode introduced by falling back to Sensor B's last known good value?**

If Sensor B has failed *permanently* (not just slow), every future fusion cycle silently reuses the same increasingly stale value from before the failure, forever — with nothing distinguishing "fresh reading" from "reading from three cycles ago" in the Fusion task's output. The system goes from an obvious failure (blocked, or an error state) to a *silent* one: it keeps producing outputs that look normal but are quietly wrong, potentially for the entire remaining runtime. This is often worse than the original problem, because a visibly broken system gets fixed; a silently degraded one doesn't, especially if nothing tracks "how many consecutive cycles has this fallback value been reused" or raises an alarm past some threshold.

**9. How to determine the right timeout value?**

It needs to be **longer than** the slowest sensor's genuine worst-case completion time under normal operation (including reasonable jitter, retries, or contention) — otherwise you'll trigger false timeouts on a perfectly healthy system, which is its own kind of bug. It needs to be **shorter than** whatever the Fusion task's own deadline or the system's tolerance for stale/missing fusion output allows — otherwise the timeout exists on paper but by the time it fires, the damage (a missed control cycle, a stale safety decision) is already done. In practice this means measuring the real worst-case sensor completion time (not guessing it) and cross-checking it against Fusion's actual downstream deadline, the same worst-case discipline as stack sizing in Exercise 10 — assumption on paper, then confirmed by measurement.

**10. Software bug vs. hardware failure — distinguishable from Fusion's point of view?**

No — from purely inside the Fusion task, a sensor that never sets its bit due to a software bug (forgot to call the set-bit function) looks *identical* to one that never sets its bit due to a dead sensor chip. The event group timeout mechanism can't tell you *why* the bit didn't get set, only *that* it didn't. Does the fix change? The immediate runtime fix (timeout + fallback) is the same either way — the system still needs to survive the missing signal regardless of cause. But the follow-up differs: a hardware failure might need a persistent fault flag, hardware diagnostics, or a maintenance alert, while a software bug needs a code fix and better test coverage of the "sensor task doesn't run to completion" path — which is a debugging/root-cause question that happens *after* the immediate runtime safety net has already done its job.

## Group E — Applying it beyond the exercise

**11. What's lost by polling three flags instead of blocking on an event group?**

Polling wastes CPU cycles checking flags that haven't changed yet, on every poll interval, for however long Fusion waits — CPU time an event-group's blocking wait gives back to other tasks entirely while genuinely idle. It also introduces a tradeoff between responsiveness and overhead: a short poll delay wastes more CPU but reacts faster once all three are ready; a long poll delay wastes less CPU but adds up to a full poll-interval's worth of unnecessary extra latency after the last sensor actually finishes. An event group's blocking wait has neither tradeoff — it uses no CPU while blocked and unblocks essentially immediately (within scheduler latency) the instant the last bit is set, which is strictly better along both axes than any polling interval you could choose.

**12. Reviewing a PR with no timeout on the event-group wait — what to flag, and why more serious here?**

Flag it as a hard blocker, not a style nitpick: without a timeout, any single sensor failure — hardware or software — permanently hangs the Fusion task, and by extension likely the rest of the system's fusion pipeline, with no path to recovery short of a full reset. This is more serious here than in a simple two-task producer/consumer queue because that queue has only *one* upstream dependency (one producer) — a single point of failure, but a single, well-understood one. This event group has **three independent upstream dependencies**, each an independent chance to hang the system forever, and the "wait for all" semantics mean the *effective* risk compounds across all three rather than resting on just one. The review comment should specifically ask: "what happens to this system, forever, the moment any one of these three sensors stops responding?" — and confirm there's a real answer, not just an assumption that it won't happen.
