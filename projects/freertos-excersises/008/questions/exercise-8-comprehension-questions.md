# Exercise 8 — Comprehension Check Questions

Companion questions for "Event group: wait for all sensors ready." Answer in your own words before checking the answer key.

---

## Group A — Understanding the core problem

1. Why does this problem need an **AND-wait on 3 independent bits**, rather than just having the Fusion task read three shared variables directly in sequence (A, then B, then C)?

2. The three sensor tasks can finish in **any order**. Why does that specific fact rule out using three sequential binary semaphore waits (`take A`, then `take B`, then `take C`) as a working design?

## Group B — Event groups vs. other mechanisms

3. Compare an event group to a **counting semaphore initialized to 3**, incremented once per sensor. Would that work as a substitute for "wait until all three are ready"? What information does the event group give you that the counting semaphore doesn't?

4. Why does the answer key say the event group carries "no data payload"? If sensor readings need to reach the Fusion task, where does the actual data go, and why isn't it put through the event group itself?

## Group C — The actual gotcha of the exercise

5. Step 5 says this exercise is built around a specific gotcha. State it precisely, in one sentence, in your own words.

6. Explain why "wait for all three sensors" is inherently riskier than "wait for one sensor" from a fault-tolerance standpoint — connect this to how many independent failure points the Fusion task now depends on.

7. The fix is described as "timeout + explicit fallback." Explain why a timeout **alone**, without a fallback, is still an incomplete fix — what would the system actually do the moment the timeout fires if there were no fallback path coded?

## Group D — Break it on paper, extended

8. Suppose the fallback, when Sensor B times out, is to proceed using Sensor B's last known good value. What new failure mode does this introduce that "wait forever" didn't have — think about what happens if Sensor B is not just slow but has failed *permanently*.

9. The answer key says Sensor B being consistently slower is "acceptable as long as it's within the timeout." How would you actually determine what the timeout value *should* be — what does it need to be longer than, and what does it need to be shorter than?

10. Suppose instead of a hardware failure, Sensor B's task simply has a bug and never calls the function that sets its event bit. From the Fusion task's point of view, is this distinguishable at all from a genuine hardware failure? Does the fix change depending on which one it actually is?

## Group E — Applying it beyond the exercise

11. A colleague suggests skipping the event group entirely and just having Fusion task poll three flags in a `while` loop with a short `vTaskDelay` between checks. What's lost by doing it this way compared to blocking on an event group?

12. If you were reviewing a PR for this system and saw an event-group wait call with **no timeout argument** (i.e., wait forever), what would you say, and why is this a more serious flag here than it might be in, say, a simple two-task producer/consumer queue?

---

## How to use this

- Group A and B check whether you understand why an event group is the right primitive, not just that it "works."
- Group C is the actual point of the exercise — if you can't state the gotcha in one clean sentence, that's a sign to reread Step 5 and rebuild the reasoning.
- Group D and E test whether you can reason about failure modes the exercise doesn't spell out explicitly.
