cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

if (NOT PICO_BOARD)
    set(PICO_BOARD cytron_maker_pi_pico CACHE STRING "RP2040 board type")
endif()

if (NOT PICO_BOARD_HEADER_DIRS)
    set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards CACHE PATH "Custom Pico board header directories")
endif()

project(cytron_maker_pi_pico_projects C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib)

option(CYTRON_BUILD_BRINGUP_TEST "Build the bring-up smoke test project" ON)
option(CYTRON_BUILD_FREERTOS_BLINKING "Build the FreeRTOS blinking demo project" ON)
option(CYTRON_BUILD_FREERTOS_EXERCISES "Build FreeRTOS exercise projects" ON)

if (CYTRON_BUILD_BRINGUP_TEST)
    add_subdirectory(projects/bringup-test)
endif()

if (CYTRON_BUILD_FREERTOS_BLINKING)
    add_subdirectory(projects/freertos-blinking)
endif()

if (CYTRON_BUILD_FREERTOS_EXERCISES AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/projects/freertos-excersises/CMakeLists.txt)
    add_subdirectory(projects/freertos-excersises)
endif()
