
## Exercise 12 — Rate Monotonic schedulability analysis

**System name:** N/A — this is a pure analysis exercise. The template's communication/task-graph steps don't apply; the content is the schedulability calculation itself.

### Step 1 — Requirements (given, not designed)

| Task | Period | WCET | Utilization (WCET/Period) |
|---|---|---|---|
| Task A | 10ms | 2ms | 0.20 |
| Task B | 20ms | 5ms | 0.25 |
| Task C | 50ms | 10ms | 0.20 |

### Step 2 — Priority assignment (RMS rule: shorter period → higher priority)

| Task | Priority |
|---|---|
| Task A | Highest |
| Task B | Middle |
| Task C | Lowest |

### Step 3 — Utilization calculation

U = 2/10 + 5/20 + 10/50 = 0.20 + 0.25 + 0.20 = **0.65**

### Step 4 — Liu & Layland bound (n = 3 tasks)

U_bound = n(2^(1/n) − 1) = 3(2^(1/3) − 1) ≈ 3(1.2599 − 1) ≈ 3(0.2599) ≈ **0.7798**

### Step 5 — Conclusion and break-it-on-paper

**Result:** U (0.65) ≤ U_bound (0.7798) → **the task set passes the sufficient bound and is guaranteed schedulable under RMS.**

Note this bound is *sufficient but not necessary* — some task sets above the bound are still schedulable when verified with exact response-time analysis. Passing this bound means you can stop here with confidence; failing it doesn't automatically mean the system is broken, just that you need the more precise check.

**Follow-up stress test (the actual "break it on paper" step):** what if Task C's WCET, measured on an unloaded system, is actually 15ms under worst-case cache/flash-wait-state conditions?

Recalculate: U = 0.20 + 0.25 + (15/50 = 0.30) = **0.75**, still under 0.7798 — passes, but barely, with much less margin than the original 0.65 suggested.

**Takeaway:** this two-minute calculation catches scheduling problems before a single line of code is written. The follow-up recalculation is the real lesson — it shows why WCET *measurement methodology* (worst-case conditions, not typical-case) matters as much as the formula itself; a task set that looks comfortably safe under optimistic WCET assumptions can be barely passing under realistic ones.