# Exercise 005

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-05-answers.md](rtos-exercises-tier1-05-answers.md)

Implemented design:

- three soft tasks that periodically check in
- one highest-priority watchdog task
- one mutex-protected heartbeat table
- watchdog feed only happens when all tasks are healthy
- one CLI polling task over USB serial

Pin usage:

- `Watchdog task` toggles `BOARD_LED_PIN` (`GP25`) after each successful feed

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/005 -B build/freertos-excersises-005-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-005-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_005.uf2`
