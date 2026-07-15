# Exercise 008

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier2-08-answers.md](rtos-exercises-tier2-08-answers.md)

Implemented design:

- three independent sensor tasks
- one event group carrying readiness bits
- one fusion task waiting for all bits with timeout
- one CLI polling task over USB serial

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/008 -B build/freertos-excersises-008-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-008-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_008.uf2`
