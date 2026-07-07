# AGENTS.md

This repository is a collection of Cytron Maker Pi Pico examples, exercises, and reusable Pico/RP2040 support code.

## Repo Layout

- `boards/`: Pico SDK custom board definitions
- `bsp/`: shared board aliases and pin naming
- `lib/`: reusable libraries shared by demos and exercises
- `projects/`: runnable example applications
- `doc/`: public-facing board documents and reference files
- `third_party/`: external dependencies such as the FreeRTOS submodule

## Build Rules

- Root build should work with:
  - `cmake -S . -B build -DPICO_SDK_PATH=$HOME/pico-sdk`
  - `cmake --build build`
- Projects should also stay buildable individually from the repo root, for example:
  - `cmake -S projects/bringup-test -B build/bringup-test -DPICO_SDK_PATH=$HOME/pico-sdk`
- Preserve the current default board unless a task explicitly requires something else:
  - `PICO_BOARD=cytron_maker_pi_pico`

## Documentation Rules

- Use repo-relative links and paths in generated docs and README files.
- Do not hardcode local machine paths such as `/home/...` or `\\wsl.localhost\...`.
- Prefer relative build commands over machine-specific absolute commands.
- Mention outputs as relative paths in backticks unless a direct link is truly needed.

## Shared Library Rules

- Put reusable code under `lib/<name>/`.
- Put public headers under `lib/<name>/include/<name>/`.
- Keep shared libraries generic unless they genuinely depend on board-specific hardware.
- `lib/pico_cli` should remain generic Pico/RP2040 infrastructure.

## CLI Rules

- New CLI commands should be added through the command table.
- Every command should include:
  - `.name`
  - `.help`
  - `.handler`
- Keep `help` useful by always providing a short help string.
- Reuse `pico_cli` helpers instead of duplicating parsing or dispatch logic.
- ANSI color output is allowed, but output should still be understandable in terminals that do not render color.

## Maintenance Notes

- Keep root and per-project README files consistent when project behavior changes.
- Preserve standalone project builds when integrating shared libraries from the repo root.
- Keep board-specific logic in demos or BSP files, and keep generic infrastructure in `lib/`.
