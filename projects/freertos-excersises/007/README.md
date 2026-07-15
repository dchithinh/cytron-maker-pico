# Exercise 007

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier2-07-answers.md](rtos-exercises-tier2-07-answers.md)

Implemented design:

- one producer task every `10 ms`
- one slower consumer task at `100-150 ms`
- bounded queue
- explicit drop-oldest overflow policy
- one CLI polling task over USB serial

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/007 -B build/freertos-excersises-007-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-007-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_007.uf2`
