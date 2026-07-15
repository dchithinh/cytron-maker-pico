## Exercise 7 — Queue overflow policy

**System name:** Rate-mismatched producer/consumer

**One-sentence description:** A fast producer (10ms) feeds a much slower, variable-latency consumer (100-150ms), guaranteeing eventual queue overflow under sustained load.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Produce item | Timer | N/A | Every 10ms | Soft | Roughly 10-15x faster than the consumer |
| Consume item | Queue receive | N/A | Every 100ms, spikes to 150ms | Soft | Structurally cannot keep up under sustained load |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| Producer task | Generate item every 10ms | Medium | Regular period | 256B | Blocks on `vTaskDelay(10ms)` |
| Consumer task | Process item, ~100-150ms per item | Medium/Low | Slower, more tolerant of lateness | 512B | Blocks on queue receive |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| Producer → Consumer | Item struct | Bounded queue, **overflow policy decided explicitly (see below)** | Rate mismatch is structural (10-15x), not occasional — overflow is guaranteed under sustained load, so the policy must be a deliberate decision tied to what the data represents |

**Overflow policy decision, with justification** (this is the actual content of the exercise):
- If items are recency-sensitive (e.g. rapidly changing sensor values) → **drop-oldest**, since a stale queued item is worse than a gap.
- If items are commands/instructions that must execute in order and none can be skipped → **drop-newest** (reject new item, keep processing what's already queued) or **block the producer** if it can tolerate the delay.
- Given the numbers in this exercise (10-15x mismatch), note explicitly: **no overflow policy actually fixes the root problem.** The real fix is questioning why the producer runs 10-15x faster than the consumer can ever drain — either slow the producer, speed up/restructure the consumer (e.g. batch processing), or increase consumer priority if it's being starved rather than genuinely slow.

### Step 4 — Task graph

```
[Producer task]
Every 10ms
      |
      | Queue (bounded, drop-oldest — see policy note above)
      v
[Consumer task]
Every 100-150ms
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** Already the baseline case in this exercise — the consumer is *always* 10-15x slower, so this question is really "what happens at steady state," and the answer is: the queue fills and the chosen overflow policy governs behavior continuously, not as a rare edge case.
2. **Two events within 1ms?** N/A — producer is a fixed 10ms timer, no burst beyond that rate.
3. **Cycle?** No.
4. **Starvation consequence?** If the queue is set to block-producer and the consumer is genuinely this slow permanently, the producer effectively runs at consumer speed — likely defeating its own purpose. This is the strongest argument for re-examining the architecture rather than just picking a clever policy.
5. **Boot-time gap?** N/A.

---