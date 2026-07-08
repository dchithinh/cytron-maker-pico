# Exercise 001

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-01-answers.md](rtos-exercises-tier1-01-answers.md)

Implemented design:

- two independent FreeRTOS tasks
- one CLI polling task over USB serial
- no queues, semaphores, or shared application data
- one LED toggles every `250 ms`
- one LED toggles every `1000 ms`
- each task blocks with `vTaskDelay(...)` as described in the answer key

Pin usage:

- `LED1 task` uses `CYTRON_GROVE6_PIN2` (`GP27`)
- `LED2 task` uses `BOARD_GROVE6_PIN1` (`GP26`)

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/001 -B build/freertos-excersises-001-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-001-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_001.uf2`
