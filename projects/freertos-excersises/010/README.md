# Exercise 010

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier2-10-answers.md](rtos-exercises-tier2-10-answers.md)

Implemented design:

- one task with nested call depth and local stack usage
- periodic reporting via `uxTaskGetStackHighWaterMark()`
- one CLI polling task over USB serial

Notes:

- The demo is meant to show measurement-based stack diagnosis.
- It uses a `512`-byte task stack as a safer starting point than the undersized example in the analysis write-up.

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/010 -B build/freertos-excersises-010-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-010-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_010.uf2`
