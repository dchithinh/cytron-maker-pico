#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

#define SENSOR_A_READY (1u << 0)
#define SENSOR_B_READY (1u << 1)
#define SENSOR_C_READY (1u << 2)
#define ALL_SENSORS_READY (SENSOR_A_READY | SENSOR_B_READY | SENSOR_C_READY)

static EventGroupHandle_t sensor_event_group;
static uint16_t latest_sensor_values[3];
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

typedef struct sensor_task_config {
    const char *name;
    EventBits_t ready_bit;
    TickType_t period_ticks;
    uint16_t base_value;
    size_t value_index;
} sensor_task_config_t;

static const sensor_task_config_t sensor_configs[] = {
    {.name = "sensor_a", .ready_bit = SENSOR_A_READY, .period_ticks = pdMS_TO_TICKS(120), .base_value = 100, .value_index = 0},
    {.name = "sensor_b", .ready_bit = SENSOR_B_READY, .period_ticks = pdMS_TO_TICKS(230), .base_value = 200, .value_index = 1},
    {.name = "sensor_c", .ready_bit = SENSOR_C_READY, .period_ticks = pdMS_TO_TICKS(350), .base_value = 300, .value_index = 2},
};

static void sensor_task(void *task_parameters) {
    const sensor_task_config_t *config = (const sensor_task_config_t *)task_parameters;
    uint16_t sample = config->base_value;

    for (;;) {
        sample += 7u;
        latest_sensor_values[config->value_index] = sample;
        xEventGroupSetBits(sensor_event_group, config->ready_bit);
        vTaskDelay(config->period_ticks);
    }
}

static void fusion_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(
            sensor_event_group,
            ALL_SENSORS_READY,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(600)
        );

        if ((bits & ALL_SENSORS_READY) == ALL_SENSORS_READY) {
            uint32_t average = ((uint32_t)latest_sensor_values[0]
                + (uint32_t)latest_sensor_values[1]
                + (uint32_t)latest_sensor_values[2]) / 3u;
            pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
                "Fusion ran with A=%u B=%u C=%u avg=%lu\r\n",
                latest_sensor_values[0],
                latest_sensor_values[1],
                latest_sensor_values[2],
                (unsigned long)average);
        } else {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Fusion timeout, ready bits=0x%02lx\r\n",
                (unsigned long)bits);
        }
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
        .prompt = "exercise-008> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 008: Event group sensor fusion gate\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Fusion waits for all three sensor-ready bits with a timeout\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    sensor_event_group = xEventGroupCreate();
    configASSERT(sensor_event_group != NULL);

    for (size_t i = 0; i < count_of(sensor_configs); ++i) {
        configASSERT(xTaskCreate(sensor_task, sensor_configs[i].name, 256, (void *)&sensor_configs[i], tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    }

    configASSERT(xTaskCreate(fusion_task, "fusion", 384, NULL, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
