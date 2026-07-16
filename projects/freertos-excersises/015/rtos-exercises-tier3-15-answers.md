
## Exercise 15 — Stream buffer vs queue for audio

**System name:** I2S audio capture with voice-activity detection
**One-sentence description:** Continuous 16kHz audio samples from I2S/DMA are forwarded to a VAD task that processes fixed-size windows not necessarily aligned to the DMA buffer size.

### Step 1 — Requirements

| Function | Trigger | Deadline | Period/frequency | Criticality | Notes |
|---|---|---|---|---|---|
| Capture audio samples | I2S DMA-complete interrupt | Must not lose samples | Continuous, ~16kHz | Soft (quality degrades if data lost, doesn't "fail") | Data is a continuous byte stream, not discrete messages |
| Run VAD on a window | Enough new bytes available | Should keep up with real-time input | Every ~20ms window | Soft | Window size may not align with DMA buffer size |

### Step 2 — Task decomposition

| Task name | Responsibility | Priority | Priority justification | Stack estimate | Blocking behavior |
|---|---|---|---|---|---|
| I2S DMA-complete ISR | Write captured samples into stream buffer | N/A (interrupt context) | Hardware-priority | N/A | Never blocks, uses ISR-safe stream buffer write |
| VAD task | Read a window's worth of bytes, run detection | Medium-high | Needs to keep pace with continuous real-time input | 1KB+ (depends on VAD algorithm) | Blocks on stream buffer read until enough bytes available |

### Step 3 — Communication design

| From → To | What's exchanged | Mechanism | Why |
|---|---|---|---|
| I2S ISR → VAD task | Continuous byte stream of audio samples | **Stream buffer**, not a regular queue | A queue is built for discrete, fixed-size messages; audio from DMA is a continuous stream with no natural message boundary. A stream buffer supports variable-length writes/reads against a shared byte stream, matching how DMA delivers audio versus how a VAD algorithm wants to consume it (e.g. 20ms windows that may not align to the DMA buffer size) |

### Step 4 — Task graph

```
[I2S DMA-complete ISR]
Writes captured samples
        |
        | Stream buffer (ISR-safe write, variable-length)
        v
[VAD task]
Reads N-byte windows, runs detection
```

### Step 5 — Break it on paper

1. **Slowest task 2x slower?** If the VAD task falls behind, the stream buffer fills — unlike a queue's discrete-item overflow, this is a byte-level backpressure situation; decide explicitly whether old samples get overwritten or the ISR write is dropped/truncated when full, since silently corrupting audio data is worse than a clean, documented drop policy.
2. **Two events within 1ms?** Not directly relevant — the DMA interrupt rate is fixed by the sample rate and buffer size, not bursty in the way a GPIO interrupt might be.
3. **Cycle?** No — one-directional flow from capture to processing.
4. **Starvation consequence?** If the VAD task is starved by higher-priority activity for too long, the stream buffer overflows and audio data is lost — a real risk if this task's priority isn't set high enough relative to its real-time input rate.
5. **Boot-time gap?** N/A — stream buffer starts empty, VAD task simply waits until the first window's worth of data arrives.

**Gotcha called out in this exercise:** stream buffers assume a single reader and single writer. If a second consumer is later added (e.g. a separate recording task also wanting the same audio stream), a single stream buffer won't fan out correctly — this would require either two independently-fed stream buffers or a different architecture (e.g. a circular buffer with independent read pointers per consumer).