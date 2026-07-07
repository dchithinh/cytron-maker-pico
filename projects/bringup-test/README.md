# Bring-Up Test

This demo is a quick hardware smoke test for the Cytron Maker Pi Pico board.

It uses the shared board support from [../../bsp/board.h](/home/thinh/workspace/cytron-maker-pico/bsp/board.h:1) and exercises:

- GPIO indicator LEDs on `GP0-GP9` and `GP26-GP27`
- User buttons on `GP20`, `GP21`, and `GP22`
- Piezo buzzer on `GP18`
- NeoPixel-compatible RGB LED on `GP28`
- USB serial logging
- USB CLI over the Pico USB serial connection

## Build From Repo Root

```bash
cmake -S /home/thinh/workspace/cytron-maker-pico -B /home/thinh/workspace/cytron-maker-pico/build -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build /home/thinh/workspace/cytron-maker-pico/build --target cytron_maker_pi_pico_hw_test
```

## Build This Demo Directly

```bash
cmake -S /home/thinh/workspace/cytron-maker-pico/projects/bringup-test -B /home/thinh/workspace/cytron-maker-pico/build/bringup-test -DPICO_SDK_PATH=$HOME/pico-sdk
cmake --build /home/thinh/workspace/cytron-maker-pico/build/bringup-test
```

## Expected Behavior

After flashing:

1. The buzzer plays a short 3-note startup chime.
2. The indicator LEDs chase one at a time.
3. The RGB LED stays dim green when no button is pressed.
4. Pressing `GP20`, `GP21`, or `GP22` changes the RGB LED color and plays a distinct tone.
5. USB serial prints button state changes.
6. Typing `boot` on the USB CLI reboots the RP2040 into UF2 bootloader mode.

## USB CLI

The demo also enables a simple shared CLI from [../../lib/pico_cli](/home/thinh/workspace/cytron-maker-pico/lib/pico_cli:1).

- Transport: Pico USB serial
- Commands: `help`, `boot`

Connect the board to your PC over USB, open the Pico serial port, then send `help` to list commands or `boot` followed by Enter to jump straight into the UF2 bootloader.

## Output

When built from the repo root, the UF2 is:

- [build/cytron_maker_pi_pico_hw_test.uf2](/home/thinh/workspace/cytron-maker-pico/build/cytron_maker_pi_pico_hw_test.uf2)
