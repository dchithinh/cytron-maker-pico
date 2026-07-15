#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

#define BUFFER_POOL_SIZE 4

typedef struct dma_buffer {
    uint8_t id;
    bool in_use;
} dma_buffer_t;

static SemaphoreHandle_t buffer_count;
static SemaphoreHandle_t free_list_mutex;
static dma_buffer_t buffers[BUFFER_POOL_SIZE];
static uint8_t free_list[BUFFER_POOL_SIZE];
static size_t free_count;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static int acquire_buffer(uint8_t *buffer_id, TickType_t wait_ticks) {
    if (xSemaphoreTake(buffer_count, wait_ticks) != pdPASS) {
        return -1;
    }

    xSemaphoreTake(free_list_mutex, portMAX_DELAY);
    configASSERT(free_count > 0u);
    *buffer_id = free_list[--free_count];
    buffers[*buffer_id].in_use = true;
    xSemaphoreGive(free_list_mutex);

    return 0;
}

static void release_buffer(uint8_t buffer_id) {
    configASSERT(buffer_id < BUFFER_POOL_SIZE);

    xSemaphoreTake(free_list_mutex, portMAX_DELAY);
    configASSERT(buffers[buffer_id].in_use);
    buffers[buffer_id].in_use = false;
    configASSERT(free_count < BUFFER_POOL_SIZE);
    free_list[free_count++] = buffer_id;
    xSemaphoreGive(free_list_mutex);

    xSemaphoreGive(buffer_count);
}

static void requester_task(void *task_parameters) {
    const char *name = (const char *)task_parameters;
    uint32_t cycle = 0;

    for (;;) {
        uint8_t buffer_id;

        if (acquire_buffer(&buffer_id, pdMS_TO_TICKS(250)) == 0) {
            pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
                "%s acquired buffer %u on cycle %lu\r\n",
                name,
                buffer_id,
                (unsigned long)cycle);
            vTaskDelay(pdMS_TO_TICKS(120 + ((cycle % 3u) * 50u)));
            release_buffer(buffer_id);
            pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
                "%s released buffer %u\r\n",
                name,
                buffer_id);
        } else {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "%s timed out waiting for a free buffer\r\n",
                name);
        }

        ++cycle;
        vTaskDelay(pdMS_TO_TICKS(80));
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
        .prompt = "exercise-009> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 009: Counting semaphore buffer pool\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Four fixed buffers managed by counting semaphore plus mutex-protected free list\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    buffer_count = xSemaphoreCreateCounting(BUFFER_POOL_SIZE, BUFFER_POOL_SIZE);
    free_list_mutex = xSemaphoreCreateMutex();
    configASSERT(buffer_count != NULL);
    configASSERT(free_list_mutex != NULL);

    free_count = BUFFER_POOL_SIZE;
    for (size_t i = 0; i < BUFFER_POOL_SIZE; ++i) {
        buffers[i].id = (uint8_t)i;
        buffers[i].in_use = false;
        free_list[i] = (uint8_t)(BUFFER_POOL_SIZE - 1u - i);
    }

    configASSERT(xTaskCreate(requester_task, "req_a", 384, (void *)"TaskA", tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(requester_task, "req_b", 384, (void *)"TaskB", tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(requester_task, "req_c", 384, (void *)"TaskC", tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
