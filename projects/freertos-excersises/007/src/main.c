#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct produced_item {
    uint32_t id;
    TickType_t produced_at;
} produced_item_t;

static QueueHandle_t item_queue;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void producer_task(void *task_parameters) {
    (void)task_parameters;
    uint32_t id = 0;

    for (;;) {
        produced_item_t item = {
            .id = id++,
            .produced_at = xTaskGetTickCount(),
        };
        produced_item_t dropped;

        if (xQueueSend(item_queue, &item, 0) != pdPASS) {
            xQueueReceive(item_queue, &dropped, 0);
            xQueueSend(item_queue, &item, 0);
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Queue full, drop-oldest policy removed item #%lu\r\n",
                (unsigned long)dropped.id);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void consumer_task(void *task_parameters) {
    (void)task_parameters;
    produced_item_t item;

    for (;;) {
        xQueueReceive(item_queue, &item, portMAX_DELAY);
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "Consumed item #%lu after %lu ms in queue\r\n",
            (unsigned long)item.id,
            (unsigned long)((xTaskGetTickCount() - item.produced_at) * portTICK_PERIOD_MS));
        vTaskDelay(pdMS_TO_TICKS(100 + ((item.id % 2u) * 50u)));
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
        .prompt = "exercise-007> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 007: Queue overflow policy\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Producer runs every 10 ms, consumer takes 100-150 ms, queue uses drop-oldest\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    item_queue = xQueueCreate(8, sizeof(produced_item_t));
    configASSERT(item_queue != NULL);

    configASSERT(xTaskCreate(producer_task, "producer", 256, NULL, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(consumer_task, "consumer", 384, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
