#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

#ifndef EXERCISE_011_FIX_LOCK_ORDER
#define EXERCISE_011_FIX_LOCK_ORDER 0
#endif

typedef struct lock_task_config {
    const char *name;
    const char *first_name;
    const char *second_name;
    SemaphoreHandle_t first_mutex;
    SemaphoreHandle_t second_mutex;
    uint32_t progress_index;
    const char *color;
} lock_task_config_t;

static SemaphoreHandle_t mutex_a;
static SemaphoreHandle_t mutex_b;
static volatile TickType_t last_progress_ticks[2];
static volatile uint32_t completed_cycles[2];
static char cli_buffer[64];
static lock_task_config_t task1_config = {
    .name = "task_1",
    .first_name = "Mutex A",
    .second_name = "Mutex B",
    .first_mutex = NULL,
    .second_mutex = NULL,
    .progress_index = 0,
    .color = PICO_CLI_ANSI_YELLOW,
};
static lock_task_config_t task2_config = {
    .name = "task_2",
#if !EXERCISE_011_FIX_LOCK_ORDER
    .first_name = "Mutex B",
    .second_name = "Mutex A",
#else
    .first_name = "Mutex A",
    .second_name = "Mutex B",
#endif
    .first_mutex = NULL,
    .second_mutex = NULL,
    .progress_index = 1,
    .color = PICO_CLI_ANSI_CYAN,
};

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void record_progress(uint32_t index) {
    completed_cycles[index]++;
    last_progress_ticks[index] = xTaskGetTickCount();
}

static void lock_task(void *task_parameters) {
    const lock_task_config_t *config = (const lock_task_config_t *)task_parameters;

    for (;;) {
        pico_cli_printf_color(config->color, "%s taking %s\r\n", config->name, config->first_name);
        xSemaphoreTake(config->first_mutex, portMAX_DELAY);

        pico_cli_printf_color(config->color,
            "%s holds %s and is requesting %s\r\n",
            config->name,
            config->first_name,
            config->second_name);
        vTaskDelay(pdMS_TO_TICKS(100));

        xSemaphoreTake(config->second_mutex, portMAX_DELAY);
        pico_cli_printf_color(config->color, "%s acquired both mutexes\r\n", config->name);

        record_progress(config->progress_index);
        vTaskDelay(pdMS_TO_TICKS(40));

        xSemaphoreGive(config->second_mutex);
        xSemaphoreGive(config->first_mutex);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void monitor_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        TickType_t now;

        vTaskDelay(pdMS_TO_TICKS(500));
        now = xTaskGetTickCount();

#if !EXERCISE_011_FIX_LOCK_ORDER
        if ((now - last_progress_ticks[0]) > pdMS_TO_TICKS(1500)
            && (now - last_progress_ticks[1]) > pdMS_TO_TICKS(1500)) {
            pico_cli_printf_color(PICO_CLI_ANSI_RED,
                "Monitor: no forward progress detected, circular-wait deadlock is active\r\n");
        }
#else
        pico_cli_printf_color(PICO_CLI_ANSI_DIM,
            "Monitor: task1 cycles=%lu task2 cycles=%lu\r\n",
            (unsigned long)completed_cycles[0],
            (unsigned long)completed_cycles[1]);
#endif
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
        .prompt = "exercise-011> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 011: Deadlock and lock ordering\r\n");
#if !EXERCISE_011_FIX_LOCK_ORDER
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Broken build: Task 1 takes A->B while Task 2 takes B->A\r\n");
#else
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Fixed build: both tasks take A->B, so circular wait cannot form\r\n");
#endif
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    mutex_a = xSemaphoreCreateMutex();
    mutex_b = xSemaphoreCreateMutex();
    configASSERT(mutex_a != NULL);
    configASSERT(mutex_b != NULL);

    task1_config.first_mutex = mutex_a;
    task1_config.second_mutex = mutex_b;
#if !EXERCISE_011_FIX_LOCK_ORDER
    task2_config.first_mutex = mutex_b;
    task2_config.second_mutex = mutex_a;
#else
    task2_config.first_mutex = mutex_a;
    task2_config.second_mutex = mutex_b;
#endif

    last_progress_ticks[0] = xTaskGetTickCount();
    last_progress_ticks[1] = xTaskGetTickCount();

    configASSERT(xTaskCreate(lock_task, task1_config.name, 384, (void *)&task1_config, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(lock_task, task2_config.name, 384, (void *)&task2_config, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(monitor_task, "monitor", 256, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
