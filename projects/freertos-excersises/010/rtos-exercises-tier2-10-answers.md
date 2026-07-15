
## Exercise 10 — Stack sizing diagnosis

**System name:** N/A — this is an analysis exercise, not a system design. The template's task/communication steps don't apply; the relevant structure is a worst-case call-path trace.

### Step 1 — Requirements (adapted: constraints, not functions)

| Constraint | Value |
|---|---|
| Allocated stack | 256 bytes |
| Local array, function 1 | 128 bytes |
| Local array, function 2 | 64 bytes |
| printf-style formatter, typical internal usage | 100+ bytes |
| Per-frame overhead (saved registers, return address, additional locals) | ~16-40 bytes per frame, not yet counted above |
| Interrupt nesting | Cortex-M shares the task stack for ISR entry by default — not yet counted above |

### Step 2 — Worst-case path trace (adapted: this replaces task decomposition)

| Call depth | Contribution | Running total |
|---|---|---|
| Function 1's frame | 128 + ~20 overhead | ~148 |
| Function 2's frame (called from 1) | 64 + ~20 overhead | ~232 |
| printf-style call (called from 2) | ~100+ | ~332+ |

**Already exceeds 256 bytes before accounting for ISR nesting on top.**

### Step 3 — Communication design
N/A — no inter-task communication involved in this analysis.

### Step 4 — Task graph
N/A — replaced by the call-path trace in Step 2.

### Step 5 — Break it on paper (adapted: verification checklist)

1. **Does static tracing alone suffice?** No — library call stack usage and compiler-generated stack use (spilling, alignment padding) aren't always visible from source inspection alone.
2. **What's the actual verification step?** Run under real load and check `uxTaskGetStackHighWaterMark()` (or equivalent) — measurement catches what static analysis misses.
3. **What margin should be added beyond the measured high-water mark?** Typically 20-30%, since worst-case paths exercised in testing don't always match worst-case paths encountered in the field.
4. **What's the corrected stack size?** Given the trace above (~332+ bytes minimum, before ISR nesting), a starting point of at least 512 bytes is reasonable, to be confirmed by actual high-water-mark measurement under load — not assumed from the arithmetic alone.
5. **Conclusion:** 256 bytes is undersized. This will overflow, likely corrupting adjacent memory (possibly another task's stack or kernel data) rather than failing cleanly and immediately.
