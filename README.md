# Cytron Maker Pi Pico Projects

This repository is a home for example firmware projects targeting the Cytron Maker Pi Pico board. Right now it includes:

- [projects/bringup-test](/home/thinh/workspace/cytron-maker-pico/projects/bringup-test/README.md:1): a board bring-up and hardware smoke test
- [projects/freertos-blinking](/home/thinh/workspace/cytron-maker-pico/projects/freertos-blinking/README.md:1): a minimal FreeRTOS demo with two blink tasks
- [projects/freertos-excersises](/home/thinh/workspace/cytron-maker-pico/projects/freertos-excersises/001/README.md:1): exercise-based FreeRTOS practice

The layout is meant to grow over time, so future board-specific projects can be added under `projects/` without cluttering the repo root.

Shared board support lives at:

- [boards/cytron_maker_pi_pico.h](/home/thinh/workspace/cytron-maker-pico/boards/cytron_maker_pi_pico.h:1)
- [bsp/board.h](/home/thinh/workspace/cytron-maker-pico/bsp/board.h:1)
- [lib](/home/thinh/workspace/cytron-maker-pico/lib/README.md:1)
- [third_party/FreeRTOS-Kernel](/home/thinh/workspace/cytron-maker-pico/third_party/FreeRTOS-Kernel:1)
- [doc](/home/thinh/workspace/cytron-maker-pico/doc:1)

Recommended top-level structure:

```text
cytron-maker-pico/
  boards/
  bsp/
  lib/
  doc/
  third_party/
  projects/
```

## Build All Projects

Configure and build the whole repo from the root:

```bash
export PICO_SDK_PATH=$HOME/pico-sdk
cmake -S . -B build
cmake --build build
```

The root build defaults to `PICO_BOARD=cytron_maker_pi_pico` and includes both demos. You can also turn demos on or off:

```bash
cmake -S . -B build \
  -DPICO_SDK_PATH=$HOME/pico-sdk \
  -DCYTRON_BUILD_BRINGUP_TEST=ON \
  -DCYTRON_BUILD_FREERTOS_BLINKING=OFF
```

## Build Individual Projects

Each demo folder is also directly buildable on its own:

```bash
cmake -S projects/bringup-test -B build/bringup-test -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/bringup-test

cmake -S projects/freertos-blinking -B build/freertos-blinking -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-blinking

cmake -S projects/freertos-excersises/001 -B build/freertos-excersises-001-standalone -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build build/freertos-excersises-001-standalone
```

## Outputs

Current UF2 outputs are:

- [build/cytron_maker_pi_pico_hw_test.uf2](/home/thinh/workspace/cytron-maker-pico/build/cytron_maker_pi_pico_hw_test.uf2)
- [build/freertos-blinking/cytron_maker_pi_pico_freertos_blink.uf2](/home/thinh/workspace/cytron-maker-pico/build/freertos-blinking/cytron_maker_pi_pico_freertos_blink.uf2)

For project-specific details, expected behavior, and pin usage, use the README in each demo folder.

Reusable shared code lives under [lib](/home/thinh/workspace/cytron-maker-pico/lib/README.md:1). The first shared module is `pico_cli`, which provides a tiny command-line interface for Pico stdio transports such as USB serial or UART. The library itself is generic Pico/RP2040 code and is not tied to this specific Cytron board.
