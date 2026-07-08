
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