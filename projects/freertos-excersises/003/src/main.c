#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct sensor_reading {
    uint32_t sequence;
    TickType_t sampled_at_ticks;
    uint16_t value;
} sensor_reading_t;

static QueueHandle_t sensor_queue;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static uint16_t fake_sensor_read(uint32_t sequence) {
    return (uint16_t)(200u + ((sequence * 37u) % 800u));
}

static void sensor_task(void *task_parameters) {
    (void)task_parameters;
    uint32_t sequence = 0;

    for (;;) {
        sensor_reading_t reading = {
            .sequence = sequence,
            .sampled_at_ticks = xTaskGetTickCount(),
            .value = fake_sensor_read(sequence),
        };
        sensor_reading_t dropped;

        if (xQueueSend(sensor_queue, &reading, 0) != pdPASS) {
            xQueueReceive(sensor_queue, &dropped, 0);
            xQueueSend(sensor_queue, &reading, 0);
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Queue full, dropped oldest sample #%lu\r\n",
                (unsigned long)dropped.sequence);
        }

        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "Sampled #%lu value=%u\r\n",
            (unsigned long)reading.sequence,
            reading.value);

        ++sequence;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void logger_task(void *task_parameters) {
    (void)task_parameters;
    sensor_reading_t reading;
    bool level = false;

    gpio_init(BOARD_GROVE6_PIN1);
    gpio_set_dir(BOARD_GROVE6_PIN1, GPIO_OUT);
    gpio_put(BOARD_GROVE6_PIN1, level);

    for (;;) {
        xQueueReceive(sensor_queue, &reading, portMAX_DELAY);

        level = !level;
        gpio_put(BOARD_GROVE6_PIN1, level);

        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "Logging #%lu value=%u sampled_at=%lu ticks\r\n",
            (unsigned long)reading.sequence,
            reading.value,
            (unsigned long)reading.sampled_at_ticks);

        vTaskDelay(pdMS_TO_TICKS(150 + ((reading.sequence % 3u) * 75u)));
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
        .prompt = "exercise-003> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 003: Sensor logger with queue\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Queue depth: ");
    printf("3 with drop-oldest policy\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Logger indicator LED: ");
    printf("GP%u\r\n", BOARD_GROVE6_PIN1);
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    sensor_queue = xQueueCreate(3, sizeof(sensor_reading_t));
    configASSERT(sensor_queue != NULL);

    BaseType_t sensor_created = xTaskCreate(
        sensor_task,
        "sensor_task",
        512,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    configASSERT(sensor_created == pdPASS);

    BaseType_t logger_created = xTaskCreate(
        logger_task,
        "logger_task",
        512,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(logger_created == pdPASS);

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
