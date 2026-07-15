# Exercise 006

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier2-06-answers.md](rtos-exercises-tier2-06-answers.md)

Implemented design:

- one medium-priority CPU-heavy task
- one selectable shared-resource path:
- raw binary semaphore to show inversion risk
- mutex to show FreeRTOS priority inheritance
- one CLI polling task over USB serial

Notes:

- Default build is the raw binary semaphore path.
- In raw mode, the low task keeps its base priority while the high task waits.
- In mutex mode, the low task prints its current priority so you can see FreeRTOS temporarily promote it.
- After the mutex is released, the logs show the priority dropping back to the base task priority.

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/006 -B build/freertos-excersises-006-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-006-standalone
```

To build the mutex variant instead:

```bash
cmake -S projects/freertos-excersises/006 -B build/freertos-excersises-006-mutex -DPICO_SDK_PATH=$HOME/pico-sdk -DEXERCISE_006_USE_MUTEX=ON
cmake --build build/freertos-excersises-006-mutex
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_006.uf2`
