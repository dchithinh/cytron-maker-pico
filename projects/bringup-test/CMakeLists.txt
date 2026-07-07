cmake_minimum_required(VERSION 3.13)

get_filename_component(CYTRON_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. REALPATH)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CYTRON_REPO_ROOT}/pico_sdk_import.cmake)

    if (NOT PICO_BOARD)
        set(PICO_BOARD cytron_maker_pi_pico CACHE STRING "RP2040 board type")
    endif()

    if (NOT PICO_BOARD_HEADER_DIRS)
        set(PICO_BOARD_HEADER_DIRS ${CYTRON_REPO_ROOT}/boards CACHE PATH "Custom Pico board header directories")
    endif()

    project(cytron_maker_pi_pico_hw_test C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    pico_sdk_init()
endif()

if (NOT TARGET pico_cli)
    add_subdirectory(${CYTRON_REPO_ROOT}/lib ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

add_executable(cytron_maker_pi_pico_hw_test
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

pico_generate_pio_header(cytron_maker_pi_pico_hw_test
    ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio
)

target_include_directories(cytron_maker_pi_pico_hw_test PRIVATE
    ${CYTRON_REPO_ROOT}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(cytron_maker_pi_pico_hw_test
    pico_cli
    pico_stdlib
    hardware_gpio
    hardware_pio
    hardware_pwm
)

pico_enable_stdio_usb(cytron_maker_pi_pico_hw_test 1)
pico_enable_stdio_uart(cytron_maker_pi_pico_hw_test 0)

pico_add_extra_outputs(cytron_maker_pi_pico_hw_test)
