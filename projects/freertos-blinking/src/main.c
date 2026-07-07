#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct led_task_config {
    const char *name;
    uint pin;
    TickType_t delay_ticks;
} led_task_config_t;

static const led_task_config_t led_tasks[] = {
    {
        .name = "status_led",
        .pin = BOARD_LED_PIN,
        .delay_ticks = pdMS_TO_TICKS(250),
    },
    {
        .name = "grove_led",
        .pin = BOARD_GROVE6_PIN1,
        .delay_ticks = pdMS_TO_TICKS(700),
    },
};

static char cli_buffer[64];
static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void blink_task(void *task_parameters) {
    const led_task_config_t *config = (const led_task_config_t *)task_parameters;
    bool level = false;

    gpio_init(config->pin);
    gpio_set_dir(config->pin, GPIO_OUT);
    gpio_put(config->pin, level);

    for (;;) {
        level = !level;
        gpio_put(config->pin, level);
        vTaskDelay(config->delay_ticks);
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
        .prompt = "freertos-blink> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "FreeRTOS blink demo for %s\r\n", BOARD_NAME);
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Task 1: ");
    printf("GP%u every 250 ms\r\n", BOARD_LED_PIN);
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Task 2: ");
    printf("GP%u every 700 ms\r\n", BOARD_GROVE6_PIN1);
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    for (size_t i = 0; i < count_of(led_tasks); ++i) {
        BaseType_t created = xTaskCreate(
            blink_task,
            led_tasks[i].name,
            256,
            (void *)&led_tasks[i],
            tskIDLE_PRIORITY + 1,
            NULL
        );

        configASSERT(created == pdPASS);
    }

    BaseType_t cli_created = xTaskCreate(
        cli_task,
        "cli_task",
        256,
        (void *)&cli,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(cli_created == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
