
## Exercise 14 — Low-power sensor node with tickless idle

**System name:** Battery-powered duty-cycled sensor node
**One-sentence description:** A node wakes every 60 seconds, samples, transmits, and returns to deep sleep, with the RTOS configured to actually reach low-power states between cycles.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Wake and sample | RTC alarm / long timer | Loose | Every 60s | Soft | Should consume minimal power between cycles |
| Transmit reading | After sampling | Loose | Every 60s | Soft | Radio power is a major budget item |
| Watchdog feed (if present) | Timer | Depends on chosen strategy | Must not force frequent wakeups | Hard (if used) | Naive short-period feed defeats duty-cycling |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Sample/transmit task | Wake, read sensor, transmit, sleep | Medium | Only task with real work most of the time | 512B | Blocks on RTC-driven wake alarm or long `vTaskDelay` |
| Watchdog task (if used) | Feed watchdog tied to duty cycle, not a fixed short timer | High | Still a hard requirement if present, but restructured for power | 128B | Feeds once before sleep and once after wake, not on a short fixed period |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| — | — | Minimal — likely no inter-task communication needed if this is a single dominant task | Simplicity is itself a power-relevant design choice: fewer tasks means fewer reasons to wake the scheduler |

### Step 4 — Task graph

```
[Sample/transmit task]
Wake (RTC alarm) -> read sensor -> transmit -> sleep (tickless idle, full 60s minus active time)
      |
      | (if used) feed watchdog just before sleep and just after wake
      v
[Watchdog task]
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If transmission occasionally takes longer (e.g. radio retry), the sleep duration for that cycle simply shortens — acceptable as long as it doesn't prevent the *next* wake from being correctly scheduled.
2. **Two events within 1ms?** N/A for this duty-cycled design; not a burst-prone system.
3. **Cycle?** The wake→sleep loop is a deliberate cycle by design, not a hazard — the exercise from the diagramming guidance about "don't draw cycles as rings" applies to visualization, not to whether the cycle itself is fine (it is).
4. **Starvation consequence?** If tickless idle isn't actually enabled, the RTOS tick interrupt fires every tick period (often 1ms) even with nothing scheduled — this alone can prevent the MCU from ever reaching deep sleep, silently destroying the power budget without any functional bug being visible.
5. **Boot-time gap?** Need to confirm which wake sources are preserved by the specific sleep mode chosen — not all sleep modes retain all wake sources (e.g. an external motion-sensor interrupt might not wake the MCU from the deepest sleep mode without specific configuration). This is easy to discover only after deployed hardware fails to wake as expected.

**Gotcha called out in this exercise:** a naive watchdog task feeding on a 1-second period (fine for Exercise 5's system) would force a wake every second here, destroying the 60-second power budget entirely. The watchdog strategy must be re-derived for duty-cycled systems — typically a longer timeout matched to the duty cycle, fed right before sleep and right after wake.