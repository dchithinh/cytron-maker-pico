# FreeRTOS Blinking

This demo sets up FreeRTOS for the Cytron Maker Pi Pico board and creates two tasks with different blink periods.

It uses:

- [../../bsp/board.h](../../bsp/board.h)
- [config/FreeRTOSConfig.h](config/FreeRTOSConfig.h)
- the FreeRTOS kernel submodule at [../../third_party/FreeRTOS-Kernel](../../third_party/FreeRTOS-Kernel)
- the shared CLI library at [../../lib/pico_cli](../../lib/pico_cli)

## Tasks

The application creates two tasks:

1. `status_led`: toggles `BOARD_LED_PIN` (`GP25`) every `250 ms`
2. `grove_led`: toggles `BOARD_GROVE6_PIN1` (`GP26`) every `700 ms`
3. `cli_task`: polls the USB serial CLI and supports `help` and `boot`

## Build From Repo Root

```bash
cmake -S . -B build -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build --target cytron_maker_pi_pico_freertos_blink
```

## Build This Demo Directly

```bash
cmake -S projects/freertos-blinking -B build/freertos-blinking -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-blinking
```

## Output

When built from the repo root, the UF2 is:

- `build/freertos-blinking/cytron_maker_pi_pico_freertos_blink.uf2`

## USB CLI

- Transport: Pico USB serial
- Commands: `help`, `boot`
