Exercise 13 — Dual-core RP2040 task partitioning

**System name:** Dual-core RP2040 motor/network/UI system
**One-sentence description:** A hard real-time motor loop, a bursty Wi-Fi stack, and a soft UI task are partitioned across RP2040's two Cortex-M0+ cores.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Motor control loop | Timer | Tight, within 1ms | Every 1ms | Hard | Needs a predictable core, minimal contention |
| Wi-Fi/networking stack | Network events | Loose | Bursty | Soft | Tolerant of delay, but has its own internal timing expectations |
| UI/display task | User interaction | Human-perceptible only | Irregular | Soft | Least time-sensitive |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Core assignment | Priority justification | Blocking behavior |
|---|---|---|---|---|---|
| Motor control task | Run PID/control logic every 1ms | Highest on its core | **Core 0**, pinned via `vTaskCoreAffinitySet` | Tightest deadline; isolated from unpredictable Core 1 activity | Blocks on 1ms timer |
| Wi-Fi stack task(s) | Handle network I/O | Higher than UI, on its core | **Core 1** | Bursty but has internal timing expectations UI doesn't | Blocks on network events |
| UI task | Update display, handle input | Lowest, on its core | **Core 1** | Least time-sensitive of the three | Blocks on display refresh timer / input events |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| UI/Wi-Fi (Core 1) → Motor control (Core 0) | Command struct (e.g. target speed) | **Single-writer lock-free double-buffer or atomic struct write**, not a standard mutex | This is cross-core shared data — a regular mutex works in the RP2040 SMP port, but cross-core lock contention is more expensive than same-core, and the hard-real-time motor task should have as few sources of unpredictable delay as possible |
| Wi-Fi task ↔ UI task (same core, Core 1) | Status/telemetry data | Standard mutex or queue | Same-core contention is cheaper; normal mechanisms are fine here |

### Step 4 — Task graph

```
Core 0                              Core 1
[Motor control task]                [Wi-Fi stack task]  [UI task]
Highest priority, isolated               |                  |
        ^                                +---(mutex/queue)--+
        |                              (same-core, normal cost)
        | Command struct
        | (lock-free/atomic, cross-core)
        |
   (from Core 1 tasks)
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If Wi-Fi stack processing spikes, it shouldn't matter to the motor loop at all, since they're on different cores — this isolation is the entire point of the core assignment.
2. **Two events within 1ms?** If both UI and Wi-Fi try to write the command struct within the same 1ms window, the single-writer assumption breaks — worth confirming only one task is ever the actual writer, or adding a small protected handoff if both need to influence the same command.
3. **Cycle?** No — command flow is one-directional (Core 1 → Core 0); no feedback path that could create a cross-core cycle.
4. **Starvation consequence?** If Core 1 is fully loaded (Wi-Fi burst + UI), it doesn't starve Core 0's motor task — that's the value of physical core isolation.
5. **Boot-time gap?** The command struct needs a sane default (e.g. "target speed = 0") before Core 1 tasks have run once, so Core 0 doesn't act on garbage during startup.

**RP2040-specific gotcha:** the two Cortex-M0+ cores have no exclusive-access load/store instructions (no LDREX/STREX) the way higher M-series cores do — true lock-free atomics are more limited than on M3/M4/M7. RP2040 provides hardware spinlocks and a hardware FIFO/mailbox mechanism between cores specifically to work around this; "lock-free" here often really means "protected by one of these hardware primitives," not "free due to instruction-level atomicity."