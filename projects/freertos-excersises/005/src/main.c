#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct task_heartbeat {
    const char *name;
    TickType_t last_check_in;
} task_heartbeat_t;

static SemaphoreHandle_t heartbeat_mutex;
static task_heartbeat_t heartbeats[3];
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void heartbeat_update(size_t index) {
    xSemaphoreTake(heartbeat_mutex, portMAX_DELAY);
    heartbeats[index].last_check_in = xTaskGetTickCount();
    xSemaphoreGive(heartbeat_mutex);
}

static bool all_tasks_healthy(TickType_t stale_after_ticks) {
    bool healthy = true;
    TickType_t now = xTaskGetTickCount();

    xSemaphoreTake(heartbeat_mutex, portMAX_DELAY);
    for (size_t i = 0; i < count_of(heartbeats); ++i) {
        if ((now - heartbeats[i].last_check_in) > stale_after_ticks) {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Watchdog detected stale task: %s\r\n",
                heartbeats[i].name);
            healthy = false;
            break;
        }
    }
    xSemaphoreGive(heartbeat_mutex);

    return healthy;
}

static void soft_task(void *task_parameters) {
    const size_t index = (size_t)task_parameters;
    const TickType_t periods[] = {
        pdMS_TO_TICKS(200),
        pdMS_TO_TICKS(400),
        pdMS_TO_TICKS(700),
    };

    for (;;) {
        heartbeat_update(index);
        vTaskDelay(periods[index]);
    }

}

static void watchdog_task(void *task_parameters) {
    (void)task_parameters;
    bool led_level = false;

    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_put(BOARD_LED_PIN, led_level);

    watchdog_enable(1500, 1);

    for (;;) {
        if (all_tasks_healthy(pdMS_TO_TICKS(1200))) {
            watchdog_update();
            led_level = !led_level;
            gpio_put(BOARD_LED_PIN, led_level);
            pico_cli_printf_color(PICO_CLI_ANSI_GREEN, "Watchdog fed after healthy task check-ins\r\n");
        } else {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Watchdog feed withheld intentionally; system will reset\r\n");
            for (;;) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
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
        .prompt = "exercise-005> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    heartbeats[0] = (task_heartbeat_t){.name = "soft_task_1", .last_check_in = 0};
    heartbeats[1] = (task_heartbeat_t){.name = "soft_task_2", .last_check_in = 0};
    heartbeats[2] = (task_heartbeat_t){.name = "soft_task_3", .last_check_in = 0};

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 005: Watchdog priority justification\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Watchdog task: ");
    printf("highest priority, feeds every 1 s if all soft tasks check in\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Watchdog LED: ");
    printf("GP%u toggles on successful feed\r\n", BOARD_LED_PIN);
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    heartbeat_mutex = xSemaphoreCreateMutex();
    configASSERT(heartbeat_mutex != NULL);

    configASSERT(xTaskCreate(soft_task, "soft_1", 256, (void *)0, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(soft_task, "soft_2", 256, (void *)1, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(soft_task, "soft_3", 256, (void *)2, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(watchdog_task, "watchdog", 384, NULL, tskIDLE_PRIORITY + 3, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
