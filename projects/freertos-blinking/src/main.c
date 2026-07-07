#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

typedef struct led_task_config {
    const char *name;
    uint pin;
    TickType_t delay_ticks;
} led_task_config_t;

static const led_task_config_t led_tasks[] = {
    {
        .name = "status_led",
        .pin = BOARD_LED_PIN,
        .delay_ticks = pdMS_TO_TICKS(250),
    },
    {
        .name = "grove_led",
        .pin = BOARD_GROVE6_PIN1,
        .delay_ticks = pdMS_TO_TICKS(700),
    },
};

static void blink_task(void *task_parameters) {
    const led_task_config_t *config = (const led_task_config_t *)task_parameters;
    bool level = false;

    gpio_init(config->pin);
    gpio_set_dir(config->pin, GPIO_OUT);
    gpio_put(config->pin, level);

    for (;;) {
        level = !level;
        gpio_put(config->pin, level);
        vTaskDelay(config->delay_ticks);
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

    printf("FreeRTOS blink demo for %s\r\n", BOARD_NAME);
    printf("Task 1: GP%u every 250 ms\r\n", BOARD_LED_PIN);
    printf("Task 2: GP%u every 700 ms\r\n", BOARD_GROVE6_PIN1);

    for (size_t i = 0; i < count_of(led_tasks); ++i) {
        BaseType_t created = xTaskCreate(
            blink_task,
            led_tasks[i].name,
            256,
            (void *)&led_tasks[i],
            tskIDLE_PRIORITY + 1,
            NULL
        );

        configASSERT(created == pdPASS);
    }

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
