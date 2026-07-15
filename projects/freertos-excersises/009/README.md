# Exercise 009

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier2-09-answers.md](rtos-exercises-tier2-09-answers.md)

Implemented design:

- counting semaphore initialized to `4`
- separate mutex-protected free list for buffer identity
- three example requester tasks acquiring and returning buffers
- one CLI polling task over USB serial

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/009 -B build/freertos-excersises-009-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-009-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_009.uf2`
