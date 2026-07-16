# Exercise 10 — Comprehension Check: Answer Key

Companion to `exercise-10-comprehension-questions.md`. Try answering first.

---

## Group A — Understanding the core problem

**1. Why ~332+ rather than 292?**

The extra amount comes from **per-frame overhead** — roughly 16-40 bytes added at *each* function call for the saved return address, saved registers the callee needs to preserve, and any compiler-added padding for alignment. With three nested calls (Function 1 → Function 2 → printf-style), that overhead is paid three separate times, adding up to roughly 40-120 bytes beyond the raw local-variable sizes. It's easy to forget because when you look at source code, you see `int array[128]` and think "that's 128 bytes" — the frame bookkeeping is invisible in the source; it's something the compiler inserts silently as part of the calling convention, not something any line of your C code explicitly asks for.

**2. Why do three sequential calls all count simultaneously, even though they're not "active" at the same time in the sense of being written to concurrently?**

Because stack usage is about **depth of the call chain at its deepest point**, not about concurrent writes. When Function 1 calls Function 2, Function 1's frame (including its 128-byte array) is still sitting on the stack — it hasn't returned yet, so its memory hasn't been reclaimed. Function 2's frame is allocated *on top of* it, not instead of it. By the time the printf-style call happens (three levels deep), all three frames — Function 1's, Function 2's, and printf's own — are simultaneously occupying stack space at once, even though only the innermost one (printf) is the code actively executing at that instant. Stack space isn't freed until each function actually *returns*, which for the deepest call hasn't happened yet.

## Group B — Why static analysis isn't enough

**3. What is register spilling, and why is it invisible in source?**

A CPU has a limited, fixed number of registers (fast, on-chip storage) available for a function to work with. If a function's logic needs to keep track of more live values simultaneously than there are registers to hold them, the compiler has to temporarily "spill" some register values out to the stack, then reload them later when needed again. This decision is made entirely by the compiler during code generation, based on register pressure at each point in the function — it's not something you write in C, and the same source code can spill differently (or not at all) depending on optimization level, compiler version, or even unrelated code changes elsewhere in the same function that shift register allocation. Because it never appears as a variable declaration or any line of source you can point to, static reading of the C code gives you zero visibility into it.

**4. Why not just read the disassembly once and trust it forever?**

Because that number is only valid for the exact compiled binary you inspected — it can silently change with any recompilation: a compiler version upgrade, a change in optimization flags, a seemingly unrelated code change elsewhere that shifts register allocation or inlining decisions, or even the same compiler making different choices due to a code change in a completely different function that happens to affect how this one gets optimized. Treating a one-time disassembly reading as a permanent guarantee assumes the build is frozen forever, which is rarely true in a project under active development — the moment anything in the toolchain or surrounding code changes, that number needs to be re-verified, not assumed to still hold.

## Group C — The verification process

**5. What does `uxTaskGetStackHighWaterMark()` actually measure?**

It returns the **minimum amount of stack that has remained unused** at the deepest point of stack usage observed so far during the task's execution — i.e., how close the task has ever come to overflowing, expressed as remaining headroom, not as bytes consumed. This distinction matters because a naive reading of "high water mark" might suggest it tracks the *maximum used* — functionally related (used = allocated - remaining), but the framing matters for interpretation: a *low* high-water-mark value is the danger signal (little headroom left, close call), not a high one. Misreading which direction is "bad" would lead you to exactly the wrong conclusion when looking at the number.

**6. Why add margin on top of a measured number? Give a concrete example of an unexercised call path.**

Because the measurement only reflects the worst-case path that actually got *exercised* during the test run — it says nothing about paths that exist in the code but weren't triggered by that particular test's inputs or timing. A concrete example: an error-handling branch that calls a verbose logging function with a large local buffer, but only triggers on a specific hardware fault condition that didn't happen to occur during the test period. The code path is real, compiled, and part of the worst-case call graph — but if the fault never occurred during testing, the high-water-mark measurement never saw it, and the "safe" number you measured is actually an undercount of the true worst case. The margin exists specifically to buffer against these known-unknowns: paths that are structurally possible but statistically unlikely to show up in any single test run.

**7. Is ISR sharing the task stack "by default" universal, or configurable?**

It's the default behavior of the underlying Cortex-M architecture's exception model (interrupts, by default, run on whichever stack pointer — MSP or PSP — was active at the moment of the interrupt), but it is not an unchangeable law — a specific RTOS port can and often does configure things differently. As discussed earlier: many RTOS ports (including FreeRTOS on Cortex-M) set up tasks to run using PSP while reserving MSP specifically for exception/interrupt handling, which — if configured this way — means ISRs use their *own* separate stack, not the interrupted task's stack. Whether this exercise's "ISR eats into task stack" assumption actually applies to a given real system depends entirely on how that specific port is configured — it's a fact to verify against your specific toolchain's documentation, not a universal constant to assume.

## Group D — Consequences and failure mode

**8. Why does overflow corrupt memory rather than fail cleanly?**

A "clean, immediate failure" would mean the hardware or RTOS actively detects the moment the stack pointer crosses its allocated boundary and immediately halts or raises a fault — this requires dedicated hardware support (e.g., a Memory Protection Unit configured with a guard region beyond the stack) or software-inserted checks, neither of which happens automatically on a typical Cortex-M without deliberate configuration. Without that protection, the stack pointer is just an ordinary register moving through ordinary RAM — nothing stops writes from proceeding into whatever memory happens to sit immediately past the allocated stack region. If that adjacent memory belongs to something else (another task's stack, kernel bookkeeping data, a global variable), the overflow silently overwrites it with whatever the overflowing function happened to push — a normal write instruction, indistinguishable at the hardware level from any other valid stack operation.

**9. What would corrupting another task's stack look like to a debugger?**

It would very likely look like a completely unrelated bug in the *victim* task — the one whose stack got silently overwritten — showing symptoms like corrupted local variables, a wrong return address causing a jump to garbage code (hard fault), or bizarre control flow, with no apparent connection to the actual task that overflowed. Someone debugging this would almost certainly suspect the *victim* task first, since that's where the visible symptom appears — the real culprit (the undersized-stack task) may have already returned normally and moved on, showing no symptoms of its own at all. This is exactly why stack overflow is one of the hardest classes of embedded bug to diagnose: the location of the symptom and the location of the actual root cause can be in two completely different, seemingly unrelated tasks, separated in both space (which memory) and time (overflow happens, then the victim task runs and crashes later).

**10. Why doesn't the RTOS just refuse undersized stack allocations automatically?**

Because the RTOS, at the point of `xTaskCreate()`, has no way to know what the task's worst-case call depth or library usage will actually be — that depends on the full call graph of code the task will execute, which can include third-party libraries, dynamically-influenced branches, and compiler-specific spilling decisions the kernel has no visibility into at task-creation time. Determining the true required stack size is exactly the hard, measurement-dependent problem this whole exercise is about — if it were something the RTOS could reliably compute automatically, this wouldn't be a skill worth an entire exercise. Some tools (static stack analyzers integrated into certain toolchains) attempt to compute worst-case bounds at compile time for exactly this reason, but they have real limitations (recursion, function pointers, and library code without source can defeat static analysis) — which is why runtime measurement remains the practical, universally-applicable fallback.

## Group E — Applying it beyond the exercise

**11. Flaw in "measured 40% free for a week, so 256 bytes would have been fine"?**

A week of runtime, however long it sounds, is still just one sample of however many possible input sequences, timing interleavings, and error conditions the system could ever encounter — it says the worst-case path *exercised during that week* stayed under a certain threshold, not that no worse path exists. Rare branches (error handling, edge-case sensor readings, a specific interrupt arriving at a specific unlucky moment relative to a specific deep call) might simply not have occurred in that particular week, exactly as discussed in Question 6. "It ran fine for a while" is evidence of typical-case behavior, not a bound on worst-case behavior — and stack sizing decisions need to be justified against the worst case, since that's precisely the scenario that, when it eventually does occur, causes silent memory corruption rather than a graceful failure.

**12. What to check before approving a PR adding a 200-byte local buffer three levels deep?**

Ask the author to trace (or better, measure) the full worst-case call depth from the task's entry point through to this new function, including this new 200-byte addition, any other local variables at each level in between, per-frame overhead, and whether this new call path could ever coincide with an ISR firing at that same depth. Then ask whether `uxTaskGetStackHighWaterMark()` has actually been checked for this task under a test run that exercises this new code path specifically (not just general system testing that might never trigger this particular function) — and whether the task's currently allocated stack size still has adequate margin (the 20-30% buffer) after accounting for this addition. The core question underlying all of this: "has anyone confirmed, by measurement under a test that actually exercises this new path, that this task's stack allocation is still sufficient — or are we just assuming it probably still fits?"
