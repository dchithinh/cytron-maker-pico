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

## Exercise 2 — Button-controlled LED via ISR

**System name:** Button-toggled LED
**One-sentence description:** A GPIO interrupt on button press toggles an LED, with the actual toggle logic deferred out of the ISR.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Detect button press | GPIO falling edge | <5ms to defer out of ISR | Async, human-triggered | Soft | ISR must stay minimal |
| Toggle LED | Semaphore signal | ~20ms (perceptible) | Async | Soft | Should feel instant to the user |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Button ISR | Detect edge, signal a task | N/A (interrupt context) | Hardware-priority, not RTOS-scheduled | N/A | Never blocks |
| Input task | Toggle LED on signal | Medium | Human-perceptible responsiveness matters | 256B | Blocks on binary semaphore |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Button ISR → Input task | Event only, no payload | Binary semaphore (`FromISR` variant) | Just a wake-up signal; ISR must never block, so only ISR-safe give is used |

### Step 4 — Task graph

```
[Button ISR]
GPIO interrupt
      |
      | Semaphore (FromISR)
      v
[Input task]
Toggles LED
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** N/A — single consumer, no queue depth to overflow.
2. **Two events within 1ms?** Mechanical bounce can fire the ISR multiple times per physical press. A binary semaphore given multiple times before the task runs still only unblocks the task once — bounce is naturally absorbed here. (If this had been a counting semaphore instead, bounce would cause multiple unwanted toggles — worth noting explicitly why binary was the right pick.)
3. **Cycle?** No — one-directional signal, no feedback path.
4. **Starvation consequence?** If Input task is starved by a higher-priority task for a long stretch, button presses feel unresponsive but nothing breaks. Acceptable given "Soft" criticality.
5. **Boot-time gap?** No shared data to initialize — semaphore starts in the "not given" state by default, which is correct (no phantom toggle on boot).

---

## Exercise 3 — Sensor logger with a queue

**System name:** Sensor-to-flash logger
**One-sentence description:** A sensor task samples periodically; a logger task with a slower, variable-latency flash write consumes the readings without losing any.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Read sensor | Timer | 50ms | Every 500ms | Soft | Fixed period |
| Write reading to flash | New data in queue | Up to 300ms occasionally | Roughly matches sensor rate | Soft | Variable latency, can lag |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Sensor task | Read sensor every 500ms, enqueue reading | Medium | Regular period, not urgent | 512B | Blocks on `vTaskDelay(500ms)` |
| Logger task | Dequeue and write to flash | Low | Longest tolerance for lateness | 1KB | Blocks on queue receive |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Sensor task → Logger task | Struct (value, timestamp) | Queue, depth 3, **drop-oldest on full** | Every reading matters, but logger can legitimately lag behind a slow flash write; bounded queue absorbs the mismatch without unbounded RAM growth |

### Step 4 — Task graph

```
[Sensor task]
Reads every 500ms
      |
      | Queue (depth 3, drop-oldest)
      v
[Logger task]
Writes to flash (up to 300ms)
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If flash writes spike to 600ms, the queue (depth 3) can fill within ~1.8s of sustained sensor input. Decided policy: drop-oldest — acceptable since this is soft real-time logging and losing an old, stale reading is preferable to blocking the sensor task or growing memory unbounded.
2. **Two events within 1ms?** N/A — sensor task is timer-driven at a fixed, slower rate; no burst risk from this source.
3. **Cycle?** No — one-directional flow, no feedback.
4. **Starvation consequence?** If Logger task never runs, queue fills and old readings get dropped — no crash, just data loss, consistent with soft criticality.
5. **Boot-time gap?** Queue starts empty — Logger task will simply block until the first reading arrives, no garbage data risk.

---

## Exercise 4 — Shared UART from two tasks

**System name:** Shared debug UART
**One-sentence description:** Two independent tasks both need to print to the same UART without corrupting each other's output.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Print debug message (Task A) | Application event | None (best-effort) | Irregular | Soft | Must not corrupt interleaved output |
| Print debug message (Task B) | Application event | None (best-effort) | Irregular | Soft | Same UART resource |

### Step 2 — Task decomposition

*(Tasks A and B are pre-existing application tasks — this exercise is purely about the shared resource, not new task creation.)*

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Task A | Existing app logic + occasional UART print | (App-defined) | (App-defined) | (App-defined) | Blocks briefly on UART mutex when printing |
| Task B | Existing app logic + occasional UART print | (App-defined) | (App-defined) | (App-defined) | Blocks briefly on UART mutex when printing |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Task A ↔ Task B (via shared resource) | Exclusive access to UART hardware | **Mutex** (not binary semaphore) | This is mutual exclusion of a resource, not event signaling — a real mutex also gets priority inheritance for free, protecting against inversion if a low-priority task holds it while a high-priority task wants to print |

### Step 4 — Task graph

```
[Task A] --\
            \--> [UART, protected by mutex]
[Task B] --/
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If Task A holds the mutex for an unusually long print (e.g. a large debug dump), Task B blocks for that whole duration. Acceptable if prints are short; if not, consider a bounded print length or moving large dumps through a queue instead.
2. **Two events within 1ms?** Both tasks could want to print near-simultaneously — the mutex serializes them correctly by design; no data corruption, just ordering (acceptable for debug output).
3. **Cycle?** No — this is a shared-resource pattern, not a producer/consumer chain.
4. **Starvation consequence?** If a high-priority task needs the mutex while a low-priority task holds it, priority inheritance (built into a real mutex) prevents the inversion scenario explored in Exercise 6.
5. **Boot-time gap?** N/A — mutex starts unlocked, first `take()` succeeds immediately.

**Gotcha called out in this exercise:** if an ISR also needs to log something, it *cannot* take this mutex — ISRs must never block. Real designs route ISR-originated log messages through a separate lock-free ring buffer or queue drained by a dedicated logging task, never sharing the UART mutex path between ISR and task contexts.

---

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
