#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct blink_task_config {
    const char *name;
    uint pin;
    TickType_t period_ticks;
} blink_task_config_t;

static const blink_task_config_t led1_task_config = {
    .name = "led1_task",
    .pin = BOARD_GROVE6_PIN2,
    .period_ticks = pdMS_TO_TICKS(250),
};

static const blink_task_config_t led2_task_config = {
    .name = "led2_task",
    .pin = BOARD_GROVE6_PIN1,
    .period_ticks = pdMS_TO_TICKS(1000),
};

static char cli_buffer[64];
static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void two_rate_blink_task(void *task_parameters) {
    const blink_task_config_t *config = (const blink_task_config_t *)task_parameters;
    bool level = false;

    gpio_init(config->pin);
    gpio_set_dir(config->pin, GPIO_OUT);
    gpio_put(config->pin, level);

    for (;;) {
        level = !level;
        gpio_put(config->pin, level);
        vTaskDelay(config->period_ticks);
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
        .prompt = "exercise-001> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 001: Two-rate blinker on %s\r\n", BOARD_NAME);
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "LED1 task: ");
    printf("GP%u every 250 ms\r\n", led1_task_config.pin);
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "LED2 task: ");
    printf("GP%u every 1000 ms\r\n", led2_task_config.pin);
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    BaseType_t led1_created = xTaskCreate(
        two_rate_blink_task,
        led1_task_config.name,
        256,
        (void *)&led1_task_config,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(led1_created == pdPASS);

    BaseType_t led2_created = xTaskCreate(
        two_rate_blink_task,
        led2_task_config.name,
        256,
        (void *)&led2_task_config,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(led2_created == pdPASS);

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
