#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct motor_command {
    uint32_t sequence;
    uint16_t target_speed;
    uint8_t source;
} motor_command_t;

static motor_command_t command_buffers[2];
static volatile uint32_t published_command_slot;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void publish_command(uint16_t target_speed, uint8_t source) {
    uint32_t inactive_slot = __atomic_load_n(&published_command_slot, __ATOMIC_RELAXED) ^ 1u;

    command_buffers[inactive_slot].sequence++;
    command_buffers[inactive_slot].target_speed = target_speed;
    command_buffers[inactive_slot].source = source;
    __atomic_store_n(&published_command_slot, inactive_slot, __ATOMIC_RELEASE);
}

static motor_command_t read_latest_command(void) {
    uint32_t slot = __atomic_load_n(&published_command_slot, __ATOMIC_ACQUIRE);
    return command_buffers[slot];
}

static void motor_control_task(void *task_parameters) {
    (void)task_parameters;
    TickType_t last_wake = xTaskGetTickCount();
    bool led_level = false;
    uint32_t loop_count = 0;

    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_put(BOARD_LED_PIN, led_level);

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
        motor_command_t command = read_latest_command();
        led_level = !led_level;
        gpio_put(BOARD_LED_PIN, led_level);

        if ((loop_count++ % 250u) == 0u) {
            pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
                "Motor loop core=%d target=%u source=%u sequence=%lu\r\n",
                get_core_num(),
                command.target_speed,
                command.source,
                (unsigned long)command.sequence);
        }
    }
}

static void wifi_task(void *task_parameters) {
    (void)task_parameters;
    uint32_t burst = 0;

    for (;;) {
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "Wi-Fi task core=%d handling burst %lu\r\n",
            get_core_num(),
            (unsigned long)burst++);
        vTaskDelay(pdMS_TO_TICKS(180));
    }
}

static void ui_task(void *task_parameters) {
    (void)task_parameters;
    uint16_t target_speed = 900;

    for (;;) {
        publish_command(target_speed, 1u);
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "UI task core=%d published target=%u rpm\r\n",
            get_core_num(),
            target_speed);
        target_speed += 50u;
        if (target_speed > 1200u) {
            target_speed = 900u;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static void cli_task(void *task_parameters) {
    pico_cli_t *cli = (pico_cli_t *)task_parameters;

    for (;;) {
        pico_cli_poll(cli);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void vApplicationMallocFailedHook(void) {
    panic("FreeRTOS malloc failed");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name) {
    (void)task;
    panic("FreeRTOS stack overflow: %s", task_name ? task_name : "unknown");
}

int main(void) {
    stdio_init_all();
    sleep_ms(1200);

    pico_cli_t cli = {
        .prompt = "exercise-013> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 013: Dual-core RP2040 partitioning\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Motor control is pinned to core 0, while Wi-Fi, UI, and CLI are pinned to core 1\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    command_buffers[0] = (motor_command_t){.sequence = 0, .target_speed = 0, .source = 0};
    command_buffers[1] = command_buffers[0];
    published_command_slot = 0u;

    configASSERT(xTaskCreateAffinitySet(motor_control_task, "motor", 384, NULL, tskIDLE_PRIORITY + 3, 1u << 0, NULL) == pdPASS);
    configASSERT(xTaskCreateAffinitySet(wifi_task, "wifi", 384, NULL, tskIDLE_PRIORITY + 2, 1u << 1, NULL) == pdPASS);
    configASSERT(xTaskCreateAffinitySet(ui_task, "ui", 384, NULL, tskIDLE_PRIORITY + 1, 1u << 1, NULL) == pdPASS);
    configASSERT(xTaskCreateAffinitySet(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, 1u << 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
