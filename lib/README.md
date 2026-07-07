# Shared Libraries

Use [lib](.) for reusable code that should be shared across multiple example or exercise projects.

Recommended structure:

```text
lib/
  my_library/
    CMakeLists.txt
    include/
      my_library/
        my_library.h
    src/
      my_library.c
```

Typical usage pattern:

1. Define the library in `lib/my_library/CMakeLists.txt`
2. Export headers from `include/`
3. Add `add_subdirectory(my_library)` in [lib/CMakeLists.txt](CMakeLists.txt)
4. Link the target from a project with `target_link_libraries(...)`

Example library CMake:

```cmake
add_library(my_library
    src/my_library.c
)

target_include_directories(my_library PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(my_library
    pico_stdlib
)
```

Example project usage:

```cmake
target_link_libraries(my_project
    pico_stdlib
    my_library
)
```

This keeps:

- `boards/` for board definitions
- `bsp/` for board support aliases/helpers
- `lib/` for reusable shared code
- `projects/` for runnable apps and exercises

Current shared libraries:

- [pico_cli](pico_cli): a tiny polled CLI for Pico stdio transports, including reusable `help` and `boot` commands. The API is designed around a command table plus shared helpers like `pico_cli_find_command()`, `pico_cli_print_help()`, and `pico_cli_execute_line()` so projects can keep extending the CLI without rewriting dispatch logic.
