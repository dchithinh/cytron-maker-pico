
## Exercise 8 — Event group: wait for all sensors ready

**System name:** Multi-sensor fusion gate
**One-sentence description:** A fusion task must wait for three independently-timed sensor tasks to each report readiness before running its calculation.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Sensor A read | Timer/hardware | Varies | Independent | Soft | Sets its own readiness bit when done |
| Sensor B read | Timer/hardware | Varies | Independent | Soft | Same |
| Sensor C read | Timer/hardware | Varies | Independent | Soft | Same |
| Fusion calculation | All three bits set | Should not wait indefinitely | Once per fusion cycle | Soft | Needs a timeout + fallback if a sensor is dead |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Sensor A/B/C tasks | Read own sensor, set own event-group bit | Medium | Independent, moderate urgency | 256B each | Blocks on own timer/hardware trigger |
| Fusion task | Wait for all 3 bits, then compute | Medium | Depends on all sensors, not itself time-critical beyond that | 512B | Blocks on event group "wait for all," **with timeout** |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Sensor A/B/C → Fusion task | Readiness signal (no data payload — data itself passed separately, e.g. via shared variables) | **Event group** (AND-wait on 3 bits) | Fusion must wait for a *combination* of independent events that may complete in any order — event groups fit this shape better than three sequential semaphore waits, which would stall if the tasks don't complete in a fixed order |

### Step 4 — Task graph

```
[Sensor A task] --(sets bit 0)--\
[Sensor B task] --(sets bit 1)----> [Event group] --(AND-wait, all 3 bits, WITH TIMEOUT)--> [Fusion task]
[Sensor C task] --(sets bit 2)--/
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If Sensor B is consistently slower, Fusion simply waits longer each cycle — acceptable as long as it's within the timeout; if not, Fusion's own deadline is at risk and the timeout/fallback path triggers.
2. **Two events within 1ms?** Not the relevant hazard here — the hazard is the *opposite*: an event that never arrives.
3. **Cycle?** No.
4. **Starvation consequence?** **This is the gotcha the exercise is built around**: if Sensor B's hardware fails and it never sets its bit, Fusion blocks forever without a timeout. The fix is a timeout on the wait call plus an explicit fallback (proceed with partial/stale data, flag an error, or enter a fault state) — never wait indefinitely on external hardware.
5. **Boot-time gap?** Event group bits should start cleared; if the fusion task could start before any sensor has run once, its first wait cycle just takes longer — not a correctness issue as long as the timeout is present.

---