#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void formatting_step(char *buffer, size_t size, uint32_t iteration) {
    char local_format_scratch[64];
    snprintf(local_format_scratch, sizeof(local_format_scratch), "iter=%lu value=%lu",
        (unsigned long)iteration,
        (unsigned long)(iteration * 17u));
    snprintf(buffer, size, "formatted payload: %s", local_format_scratch);
}

static void analysis_step(uint32_t iteration) {
    char local_payload[128];
    formatting_step(local_payload, sizeof(local_payload), iteration);
    pico_cli_printf_color(PICO_CLI_ANSI_YELLOW, "%s\r\n", local_payload);
}

static void stack_diagnosis_task(void *task_parameters) {
    (void)task_parameters;
    uint32_t iteration = 0;

    for (;;) {
        analysis_step(iteration++);
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "Stack high-water mark: %lu words (%lu bytes)\r\n",
            (unsigned long)watermark,
            (unsigned long)(watermark * sizeof(StackType_t)));
        if ((watermark * sizeof(StackType_t)) < 128u) {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Warning: remaining stack margin is getting small\r\n");
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
        .prompt = "exercise-010> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 010: Stack sizing diagnosis\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Nested calls plus formatting are used, then uxTaskGetStackHighWaterMark() reports margin\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    configASSERT(xTaskCreate(stack_diagnosis_task, "stack_diag", 256, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
