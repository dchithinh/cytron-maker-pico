#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

#ifndef EXERCISE_006_USE_MUTEX
#define EXERCISE_006_USE_MUTEX 0
#endif

static SemaphoreHandle_t raw_lock;
static SemaphoreHandle_t mutex_lock;
static char cli_buffer[64];
static TaskHandle_t low_raw_task_handle;
static TaskHandle_t high_raw_task_handle;
static TaskHandle_t low_mutex_task_handle;
static TaskHandle_t high_mutex_task_handle;
static TaskHandle_t medium_task_handle;

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};


static void medium_task(void *task_parameters) {
    (void)task_parameters;
    volatile uint32_t spin = 0;

    for (;;) {
        for (uint32_t i = 0; i < 60000u; ++i) {
            spin += i;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#if !EXERCISE_006_USE_MUTEX
static void low_raw_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        xSemaphoreTake(raw_lock, portMAX_DELAY);
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "[raw] low task took binary semaphore at priority %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        for (int i = 0; i < 5; ++i) {
            pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
                "[raw] low task holding semaphore, slice=%d, current priority=%lu\r\n",
                i + 1,
                (unsigned long)uxTaskPriorityGet(NULL));
            vTaskDelay(pdMS_TO_TICKS(40));
        }
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "[raw] low task releasing semaphore from priority %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        xSemaphoreGive(raw_lock);
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW,
            "[raw] low task released semaphore, priority stays %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void high_raw_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        TickType_t start = xTaskGetTickCount();
        pico_cli_printf_color(PICO_CLI_ANSI_RED,
            "[raw] high task requesting semaphore; low=%lu medium=%lu high=%lu\r\n",
            (unsigned long)uxTaskPriorityGet(low_raw_task_handle),
            (unsigned long)uxTaskPriorityGet(medium_task_handle),
            (unsigned long)uxTaskPriorityGet(high_raw_task_handle));
        xSemaphoreTake(raw_lock, portMAX_DELAY);
        TickType_t wait = xTaskGetTickCount() - start;
        pico_cli_printf_color(PICO_CLI_ANSI_RED,
            "[raw] high task acquired semaphore after %lu ms; low still=%lu\r\n",
            (unsigned long)(wait * portTICK_PERIOD_MS),
            (unsigned long)uxTaskPriorityGet(low_raw_task_handle));
        xSemaphoreGive(raw_lock);
        vTaskDelay(pdMS_TO_TICKS(180));
    }
}
#else
static void low_mutex_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        xSemaphoreTake(mutex_lock, portMAX_DELAY);
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "[fix] low task took mutex at priority %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        for (int i = 0; i < 5; ++i) {
            UBaseType_t current_priority = uxTaskPriorityGet(NULL);
            pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
                "[fix] low task holding mutex, slice=%d, current priority=%lu\r\n",
                i + 1,
                (unsigned long)current_priority);
            vTaskDelay(pdMS_TO_TICKS(40));
        }
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "[fix] low task releasing mutex from priority %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        xSemaphoreGive(mutex_lock);
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "[fix] low task released mutex, priority restored to %lu\r\n",
            (unsigned long)uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void high_mutex_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        TickType_t start = xTaskGetTickCount();
        pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
            "[fix] high task requesting mutex; low=%lu medium=%lu high=%lu\r\n",
            (unsigned long)uxTaskPriorityGet(low_mutex_task_handle),
            (unsigned long)uxTaskPriorityGet(medium_task_handle),
            (unsigned long)uxTaskPriorityGet(high_mutex_task_handle));
        xSemaphoreTake(mutex_lock, portMAX_DELAY);
        TickType_t wait = xTaskGetTickCount() - start;
        pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
            "[fix] high task acquired mutex after %lu ms; low now=%lu\r\n",
            (unsigned long)(wait * portTICK_PERIOD_MS),
            (unsigned long)uxTaskPriorityGet(low_mutex_task_handle));
        xSemaphoreGive(mutex_lock);
        vTaskDelay(pdMS_TO_TICKS(180));
    }
}

#endif

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
        .prompt = "exercise-006> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 006: Priority inversion / inheritance demonstrator\r\n");
#if !EXERCISE_006_USE_MUTEX
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "This build runs the raw binary semaphore path, so the low task will not be priority-boosted\r\n");
#else
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "This build runs the mutex path and prints the low task priority while it holds the mutex\r\n");
#endif
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    raw_lock = xSemaphoreCreateBinary();
    mutex_lock = xSemaphoreCreateMutex();
    configASSERT(raw_lock != NULL);
    configASSERT(mutex_lock != NULL);
    xSemaphoreGive(raw_lock);

#if !EXERCISE_006_USE_MUTEX
    configASSERT(xTaskCreate(low_raw_task, "low_raw", 384, NULL, tskIDLE_PRIORITY + 1, &low_raw_task_handle) == pdPASS);
    configASSERT(xTaskCreate(high_raw_task, "high_raw", 384, NULL, tskIDLE_PRIORITY + 3, &high_raw_task_handle) == pdPASS);
#else
    configASSERT(xTaskCreate(low_mutex_task, "low_fix", 384, NULL, tskIDLE_PRIORITY + 1, &low_mutex_task_handle) == pdPASS);
    configASSERT(xTaskCreate(high_mutex_task, "high_fix", 384, NULL, tskIDLE_PRIORITY + 3, &high_mutex_task_handle) == pdPASS);
#endif

    configASSERT(xTaskCreate(medium_task, "medium", 384, NULL, tskIDLE_PRIORITY + 2, &medium_task_handle) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
