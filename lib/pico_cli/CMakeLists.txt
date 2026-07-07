add_library(pico_cli
    src/pico_cli.c
)

target_include_directories(pico_cli PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pico_cli
    pico_stdlib
    pico_bootrom
)
