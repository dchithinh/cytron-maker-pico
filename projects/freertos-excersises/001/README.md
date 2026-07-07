# Exercise 001

This folder contains the implementation for the exercise described in:

- [rtos-exercises-tier1-answers.md](/home/thinh/workspace/cytron-maker-pico/projects/freertos-excersises/001/rtos-exercises-tier1-answers.md:1)

Implemented design:

- two independent FreeRTOS tasks
- no queues, semaphores, or shared application data
- one LED toggles every `250 ms`
- one LED toggles every `1000 ms`
- each task blocks with `vTaskDelay(...)` as described in the answer key

Pin usage:

- `LED1 task` uses `BOARD_LED_PIN` (`GP25`)
- `LED2 task` uses `BOARD_GROVE6_PIN1` (`GP26`)

## Build This Exercise Directly

```bash
cmake -S /home/thinh/workspace/cytron-maker-pico/projects/freertos-excersises/001 -B /home/thinh/workspace/cytron-maker-pico/build/freertos-excersises-001-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build /home/thinh/workspace/cytron-maker-pico/build/freertos-excersises-001-standalone
```

## Output

When built directly, the UF2 will be placed under the chosen build directory as:

- `cytron_maker_pi_pico_freertos_exercise_001.uf2`
