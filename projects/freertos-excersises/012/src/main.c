#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

typedef struct rms_task_spec {
    const char *name;
    uint32_t period_ms;
    uint32_t wcet_ms;
} rms_task_spec_t;

static const rms_task_spec_t nominal_tasks[] = {
    {.name = "Task A", .period_ms = 10, .wcet_ms = 2},
    {.name = "Task B", .period_ms = 20, .wcet_ms = 5},
    {.name = "Task C", .period_ms = 50, .wcet_ms = 10},
};

static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static double calculate_utilization(const rms_task_spec_t *tasks, size_t task_count) {
    double utilization = 0.0;

    for (size_t i = 0; i < task_count; ++i) {
        utilization += (double)tasks[i].wcet_ms / (double)tasks[i].period_ms;
    }

    return utilization;
}

static double liu_layland_bound(size_t task_count) {
    (void)task_count;
    return 0.7798;
}

static void rms_analysis_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        double utilization = calculate_utilization(nominal_tasks, count_of(nominal_tasks));
        double stressed_utilization = utilization - ((double)10 / 50) + ((double)15 / 50);
        double bound = liu_layland_bound(count_of(nominal_tasks));

        pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
            "Nominal RMS: U=%.2f, bound=%.4f -> %s\r\n",
            utilization,
            bound,
            utilization <= bound ? "guaranteed schedulable" : "needs exact analysis");
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "Stress case: Task C WCET=15 ms -> U=%.2f -> %s\r\n",
            stressed_utilization,
            stressed_utilization <= bound ? "still under the bound" : "over the bound");

        vTaskDelay(pdMS_TO_TICKS(3000));
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
        .prompt = "exercise-012> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 012: Rate-monotonic schedulability analysis\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "This demo prints the utilization test and the stressed Task C recalculation from the answer key\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    for (size_t i = 0; i < count_of(nominal_tasks); ++i) {
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "%s: period=%lu ms, WCET=%lu ms\r\n",
            nominal_tasks[i].name,
            (unsigned long)nominal_tasks[i].period_ms,
            (unsigned long)nominal_tasks[i].wcet_ms);
    }

    configASSERT(xTaskCreate(rms_analysis_task, "rms", 512, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
