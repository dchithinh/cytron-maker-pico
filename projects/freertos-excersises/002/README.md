# Exercise 002

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-02-answers.md](rtos-exercises-tier1-02-answers.md)

Implemented design:

- one GPIO interrupt on `BOARD_BUTTON1_PIN`
- one binary semaphore from ISR to task
- one input task that toggles the LED outside interrupt context
- one CLI polling task over USB serial

Pin usage:

- `Button ISR` uses `BOARD_BUTTON1_PIN` (`GP20`)
- `Input task` toggles `BOARD_LED_PIN` (`GP25`)

USB CLI:

- Transport: Pico USB serial
- Commands: `help`, `boot`

## Build This Exercise Directly

```bash
cmake -S projects/freertos-excersises/002 -B build/freertos-excersises-002-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-002-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_002.uf2`
