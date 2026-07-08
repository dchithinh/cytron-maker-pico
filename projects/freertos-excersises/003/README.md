# Exercise 003

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-03-answers.md](rtos-exercises-tier1-03-answers.md)

Implemented design:

- one periodic sensor task every `500 ms`
- one logger task consuming readings from a queue
- queue depth `3`
- drop-oldest policy when the queue is full
- one CLI polling task over USB serial

Pin usage:

- `Logger task` toggles `BOARD_GROVE6_PIN1` (`GP26`) while processing samples

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/003 -B build/freertos-excersises-003-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-003-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_003.uf2`
