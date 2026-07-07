# Cytron Maker Pi Pico Projects

This repository is a home for example firmware projects targeting the Cytron Maker Pi Pico board. Right now it includes:

- [projects/bringup-test](projects/bringup-test/README.md): a board bring-up and hardware smoke test
- [projects/freertos-blinking](projects/freertos-blinking/README.md): a minimal FreeRTOS demo with two blink tasks
- [projects/freertos-excersises](projects/freertos-excersises/001/README.md): exercise-based FreeRTOS practice

The layout is meant to grow over time, so future board-specific projects can be added under `projects/` without cluttering the repo root.

Shared board support lives at:

- [boards/cytron_maker_pi_pico.h](boards/cytron_maker_pi_pico.h)
- [bsp/board.h](bsp/board.h)
- [lib](lib/README.md)
- [third_party/FreeRTOS-Kernel](third_party/FreeRTOS-Kernel)
- [doc](doc)

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
