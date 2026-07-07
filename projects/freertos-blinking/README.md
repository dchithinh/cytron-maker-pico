# FreeRTOS Blinking

This demo sets up FreeRTOS for the Cytron Maker Pi Pico board and creates two tasks with different blink periods.

It uses:

- [../../bsp/board.h](/home/thinh/workspace/cytron-maker-pico/bsp/board.h:1)
- [config/FreeRTOSConfig.h](/home/thinh/workspace/cytron-maker-pico/projects/freertos-blinking/config/FreeRTOSConfig.h:1)
- the FreeRTOS kernel submodule at [../../third_party/FreeRTOS-Kernel](/home/thinh/workspace/cytron-maker-pico/third_party/FreeRTOS-Kernel:1)

## Tasks

The application creates two tasks:

1. `status_led`: toggles `BOARD_LED_PIN` (`GP25`) every `250 ms`
2. `grove_led`: toggles `BOARD_GROVE6_PIN1` (`GP26`) every `700 ms`

## Build From Repo Root

```bash
cmake -S /home/thinh/workspace/cytron-maker-pico -B /home/thinh/workspace/cytron-maker-pico/build -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build /home/thinh/workspace/cytron-maker-pico/build --target cytron_maker_pi_pico_freertos_blink
```

## Build This Demo Directly

```bash
cmake -S /home/thinh/workspace/cytron-maker-pico/projects/freertos-blinking -B /home/thinh/workspace/cytron-maker-pico/build/freertos-blinking -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build /home/thinh/workspace/cytron-maker-pico/build/freertos-blinking
```

## Output

When built from the repo root, the UF2 is:

- [build/freertos-blinking/cytron_maker_pi_pico_freertos_blink.uf2](/home/thinh/workspace/cytron-maker-pico/build/freertos-blinking/cytron_maker_pi_pico_freertos_blink.uf2)
