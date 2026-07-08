# Exercise 004

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-04-answers.md](rtos-exercises-tier1-04-answers.md)

Implemented design:

- two independent tasks both writing to the same debug serial output
- one FreeRTOS mutex protecting the shared UART/stdio path
- one CLI polling task over USB serial

Notes:

- Task output is serialized through a mutex to prevent interleaved log lines.
- The CLI still runs over USB serial for consistency across the repo.

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/004 -B build/freertos-excersises-004-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-004-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_004.uf2`
