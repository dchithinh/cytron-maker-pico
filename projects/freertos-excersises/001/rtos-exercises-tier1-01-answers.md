# Tier 1 Answer Key — Foundations

Each answer follows the same steps as `rtos-design-template.md`. Compare your own filled-in template against this section by section, not just against the final conclusion.

---

## Exercise 1 — Two-rate blinker

**System name:** Two-rate blinker
**One-sentence description:** Two LEDs blink independently at different fixed rates, with no shared data or interrupts.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Toggle LED1 | Timer | ~50ms (cosmetic only) | Every 250ms | Soft | No consequence if a few ms late |
| Toggle LED2 | Timer | ~50ms (cosmetic only) | Every 1000ms | Soft | No consequence if a few ms late |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| LED1 task | Toggle LED1 every 250ms | Low | No deadline pressure at all | 128B | Blocks on `vTaskDelay(250ms)` |
| LED2 task | Toggle LED2 every 1000ms | Low | Same as above | 128B | Blocks on `vTaskDelay(1000ms)` |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| — | — | **None** | Neither task depends on the other's data or timing at all |

### Step 4 — Task graph

```
[LED1 task]           [LED2 task]
Toggles every 250ms    Toggles every 1000ms
```
No arrows — that's correct here, not an omission.

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** Irrelevant — nothing depends on either task's timing.
2. **Two events within 1ms?** N/A, no external events.
3. **Cycle in the graph?** No graph edges exist, so no.
4. **Starvation consequence?** Worst case, an LED blinks a bit irregularly. No real-world impact.
5. **Boot-time gap?** Both tasks start independently; no shared state to initialize.

**Takeaway:** the exercise is testing whether you add unnecessary synchronization out of habit. Correct answer here is "no mechanism needed" — recognizing that is the skill.

---
