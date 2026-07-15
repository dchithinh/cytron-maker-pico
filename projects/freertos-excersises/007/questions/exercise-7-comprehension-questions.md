# Exercise 7 — Comprehension Check Questions

Companion questions for "Queue overflow policy." Answer in your own words before checking the answer key.

---

## Group A — Understanding the core problem

1. The producer runs every 10ms and the consumer takes 100-150ms per item. Explain in your own words why overflow is **guaranteed**, not just possible, under sustained load — do the actual math to show it.

2. Why does the exercise call this a "structural" mismatch rather than an occasional slowdown? What's the practical difference between those two framings when you're deciding how to respond to it?

## Group B — Choosing an overflow policy

3. What specific property of the data (not the queue mechanism) determines whether you should choose drop-oldest vs. drop-newest vs. block-the-producer?

4. Give an example of a data type where **drop-oldest** would be actively dangerous or wrong, contradicting the sensor-value example in the answer key.

5. If you choose "block the producer" as your overflow policy, what happens to the producer's own 10ms timing guarantee? Is this actually a valid choice given the requirements table in Step 1?

## Group C — The deeper point of the exercise

6. The answer key states: "no overflow policy actually fixes the root problem." Explain what it means for a fix to be **cosmetic** versus **structural** in this context.

7. List two concrete architectural changes (not overflow policies) that would actually address the 10-15x rate mismatch, and explain what each one trades off.

8. Suppose you increase the consumer's priority instead of restructuring anything else. Under what condition does this actually help, and under what condition does it do nothing (or make things worse)?

## Group D — Break it on paper, extended

9. The answer key says block-the-producer risks "defeating its own purpose." Construct a concrete scenario (with a real-world producer, like a sensor) where this failure mode would be genuinely bad for the system, not just theoretically inelegant.

10. Suppose the consumer's 150ms spike isn't random — it happens specifically when a lower-priority task hogs the CPU. Does this exercise's problem now overlap with a concept from Exercise 6? Explain the connection or the difference.

11. If this system needs to run for weeks unattended, what would you actually *measure* in production to confirm your chosen overflow policy is behaving as intended, rather than assuming the design is correct because it made sense on paper?

## Group E — Applying it beyond the exercise

12. A colleague proposes "just make the queue bigger" as the fix for this mismatch. Explain precisely why this doesn't solve the problem, using the actual numbers from the exercise.

13. If you were reviewing a PR that added a producer/consumer queue with no explicit overflow policy stated anywhere (just relying on the RTOS default behavior), what would you flag, and why does "relying on the default" count as a design decision whether or not anyone intended it to be one?

---

## How to use this

- Group A and B check whether you understood the exercise as written.
- Group C is the actual point of the exercise — if you can only describe overflow *policies* but not explain why none of them are a real fix, go back and reread Step 5's framing.
- Group D and E test transfer to scenarios not explicitly in the exercise, closer to what a real design review or interview would probe.
