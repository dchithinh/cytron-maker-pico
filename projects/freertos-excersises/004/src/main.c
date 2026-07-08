#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

static SemaphoreHandle_t uart_mutex;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void uart_logf(const char *tag, const char *color, const char *format, ...) {
    va_list args;

    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    pico_cli_printf_color(color, "[%s] ", tag);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\r\n");
    xSemaphoreGive(uart_mutex);
}

static void task_a(void *task_parameters) {
    (void)task_parameters;
    uint32_t counter = 0;

    for (;;) {
        uart_logf("TaskA", PICO_CLI_ANSI_CYAN, "event %lu using shared UART safely", (unsigned long)counter++);
        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

static void task_b(void *task_parameters) {
    (void)task_parameters;
    uint32_t counter = 0;

    for (;;) {
        uart_logf("TaskB", PICO_CLI_ANSI_YELLOW, "event %lu using shared UART safely", (unsigned long)counter++);
        vTaskDelay(pdMS_TO_TICKS(1100));
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
        .prompt = "exercise-004> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 004: Shared UART from two tasks\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "TaskA and TaskB serialize their output through a FreeRTOS mutex\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    uart_mutex = xSemaphoreCreateMutex();
    configASSERT(uart_mutex != NULL);

    configASSERT(xTaskCreate(task_a, "task_a", 384, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(task_b, "task_b", 384, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
