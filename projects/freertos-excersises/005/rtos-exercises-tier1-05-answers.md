
## Exercise 5 — Watchdog priority justification

**System name:** Watchdog-supervised control system
**One-sentence description:** A hard real-time watchdog-feed task coexists with three soft real-time tasks of varying periods.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Feed watchdog | Timer | 10ms | Every 1s | **Hard** | Missed = uncontrolled MCU reset |
| Soft task 1 | Timer | Varies | Longer period | Soft | Degraded quality if late, not failure |
| Soft task 2 | Timer | Varies | Longer period | Soft | Same |
| Soft task 3 | Timer | Varies | Longer period | Soft | Same |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Watchdog task | Check other tasks' health, then feed hardware watchdog | **Highest** | Consequence of missing this deadline (uncontrolled reset) is categorically worse than any soft-task's missed deadline | 256B | Blocks on 1s timer |
| Soft task 1/2/3 | Application-specific | Medium/Low, ranked by their own deadlines | Assigned relative to each other using RMS reasoning (shorter period → higher priority) | App-defined | App-defined |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Soft tasks → Watchdog task | Last-checked-in timestamp per task | Mutex-protected shared variables (one per monitored task), or a shared status struct | Watchdog task must confirm other critical tasks are actually alive, not just feed on its own timer unconditionally |

### Step 4 — Task graph

```
[Soft task 1] --\
[Soft task 2] ----> [Watchdog task] --> Hardware watchdog feed
[Soft task 3] --/    Checks all check-ins
                       before feeding
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If a soft task's check-in becomes stale (task hung), the watchdog task should detect this and *withhold* the feed, deliberately allowing the hardware watchdog to reset the system — that's the intended fail-safe, not a bug.
2. **Two events within 1ms?** N/A — watchdog checks are on a fixed timer.
3. **Cycle?** No — one-directional health reporting into the watchdog task.
4. **Starvation consequence?** This IS the worst-case-consequence task in the system by design — it's given highest priority specifically because its starvation causes the worst outcome (uncontrolled reset).
5. **Boot-time gap?** Check-in timestamps need sane initial values at boot (e.g. current tick count, not zero/garbage) so the watchdog doesn't immediately conclude a task is "stale" before it's had a chance to run once.

**Gotcha called out in this exercise:** a naive watchdog-feed task that pets the watchdog unconditionally on its own timer only proves *that task* is alive — it doesn't detect a hang in any other task. Making the feed conditional on other tasks' health is what makes the watchdog actually useful as a system-wide safety net, not just a self-check.