cmake_minimum_required(VERSION 3.13)

get_filename_component(CYTRON_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../.. REALPATH)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(${CYTRON_REPO_ROOT}/pico_sdk_import.cmake)

    if (NOT PICO_BOARD)
        set(PICO_BOARD cytron_maker_pi_pico CACHE STRING "RP2040 board type")
    endif()

    if (NOT PICO_BOARD_HEADER_DIRS)
        set(PICO_BOARD_HEADER_DIRS ${CYTRON_REPO_ROOT}/boards CACHE PATH "Custom Pico board header directories")
    endif()

    project(cytron_maker_pi_pico_freertos_exercise_015 C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    pico_sdk_init()
endif()

if (NOT TARGET pico_cli)
    add_subdirectory(${CYTRON_REPO_ROOT}/lib ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

set(FREERTOS_KERNEL_PATH ${CYTRON_REPO_ROOT}/third_party/FreeRTOS-Kernel)

add_library(freertos_exercise_015_config INTERFACE)
target_include_directories(freertos_exercise_015_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/config
)
target_compile_definitions(freertos_exercise_015_config INTERFACE
    projCOVERAGE_TEST=0
)

add_subdirectory(
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040
    ${CMAKE_CURRENT_BINARY_DIR}/freertos_kernel
)

add_executable(cytron_maker_pi_pico_freertos_exercise_015
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_include_directories(cytron_maker_pi_pico_freertos_exercise_015 PRIVATE
    ${CYTRON_REPO_ROOT}
)

target_link_libraries(cytron_maker_pi_pico_freertos_exercise_015
    pico_cli
    pico_stdlib
    FreeRTOS-Kernel-Heap4
    freertos_exercise_015_config
)

pico_enable_stdio_usb(cytron_maker_pi_pico_freertos_exercise_015 1)
pico_enable_stdio_uart(cytron_maker_pi_pico_freertos_exercise_015 0)

pico_add_extra_outputs(cytron_maker_pi_pico_freertos_exercise_015)
