# Exercise 10 — Comprehension Check Questions

Companion questions for "Stack sizing diagnosis." Answer in your own words before checking the answer key.

---

## Group A — Understanding the core problem

1. Walk through the arithmetic yourself: why does the running total reach ~332+ bytes rather than just 128+64+100=292? What's contributing the extra amount, and why is it easy to forget when eyeballing the code?

2. The three local arrays/buffers (128, 64, and the printf internal usage) never exist "at the same time" in the sense of being simultaneously *written to* by unrelated code — they belong to three different function calls. Explain why they still all count against the stack budget simultaneously at the deepest point of the call chain, rather than being counted separately.

## Group B — Why static analysis isn't enough

3. Step 5 says static tracing alone doesn't suffice, citing "compiler-generated stack use (spilling, alignment padding)." Explain in your own words what register spilling is and why it consumes stack space that isn't visible anywhere in the C source code.

4. Why can't you just read the disassembly/compiled output once and trust that number forever, instead of measuring at runtime with `uxTaskGetStackHighWaterMark()`?

## Group C — The verification process

5. Explain what `uxTaskGetStackHighWaterMark()` actually measures — is it telling you how much stack has been used, or how much is left unused? Why does that distinction matter for how you interpret the number it returns?

6. Why is a 20-30% margin added *on top of* a measured high-water mark, rather than just using the measured number directly? What does "worst-case paths in testing don't always match worst-case paths in the field" mean concretely — give an example of a call path that might exist in the code but never get exercised during normal testing.

7. The interrupt nesting note says Cortex-M shares the task stack for ISR entry "by default." What does "by default" imply here — is this always true regardless of configuration, or is it something a specific RTOS port might change? Connect this to the earlier discussion about MSP/PSP.

## Group D — Consequences and failure mode

8. Explain precisely why stack overflow is described as "likely corrupting adjacent memory... rather than failing cleanly and immediately." What would a "clean, immediate failure" have looked like instead, and why doesn't a plain stack overflow behave that way by default?

9. Suppose the 256-byte stack overflow in this exercise corrupts the stack of a *different*, unrelated task rather than kernel data. Describe what the resulting bug would likely look like to someone debugging it — would they suspect the real task (the one with the undersized stack) first, or something else? Why does this make the bug hard to diagnose?

10. Given the corruption risk, why doesn't FreeRTOS (or most RTOSes) just refuse to let you allocate an undersized stack in the first place — why is this left to the developer to get right via measurement, rather than being enforced automatically?

## Group E — Applying it beyond the exercise

11. A colleague says: "I ran the system for a full week under heavy load and `uxTaskGetStackHighWaterMark()` never dropped below 40% free, so I'm confident 256 bytes would have been fine here." What's the flaw in using only that evidence to justify a tight stack size, even with real measured data?

12. If you were reviewing a PR that added a new function call three levels deep into an existing task's call chain — a function that itself declares a 200-byte local buffer — what specifically would you ask the author to check before approving, based on everything this exercise demonstrates?

---

## How to use this

- Group A and B test whether you understand *why* the naive arithmetic undercounts, not just that it does.
- Group C is the actual discipline this exercise teaches: measure, don't assume — these questions check whether you understand what's being measured and why margin is still needed on top of a real number.
- Group D and E push into the failure mode and its real-world debugging consequences, which is where this becomes an interview-relevant story rather than a math exercise.
