#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

typedef struct blink_task_config {
    const char *name;
    uint pin;
    TickType_t period_ticks;
} blink_task_config_t;

static const blink_task_config_t led1_task_config = {
    .name = "led1_task",
    .pin = BOARD_LED_PIN,
    .period_ticks = pdMS_TO_TICKS(250),
};

static const blink_task_config_t led2_task_config = {
    .name = "led2_task",
    .pin = BOARD_GROVE6_PIN1,
    .period_ticks = pdMS_TO_TICKS(1000),
};

static void two_rate_blink_task(void *task_parameters) {
    const blink_task_config_t *config = (const blink_task_config_t *)task_parameters;
    bool level = false;

    gpio_init(config->pin);
    gpio_set_dir(config->pin, GPIO_OUT);
    gpio_put(config->pin, level);

    for (;;) {
        level = !level;
        gpio_put(config->pin, level);
        vTaskDelay(config->period_ticks);
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

    printf("Exercise 001: Two-rate blinker on %s\r\n", BOARD_NAME);
    printf("LED1 task: GP%u every 250 ms\r\n", BOARD_LED_PIN);
    printf("LED2 task: GP%u every 1000 ms\r\n", BOARD_GROVE6_PIN1);

    BaseType_t led1_created = xTaskCreate(
        two_rate_blink_task,
        led1_task_config.name,
        256,
        (void *)&led1_task_config,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(led1_created == pdPASS);

    BaseType_t led2_created = xTaskCreate(
        two_rate_blink_task,
        led2_task_config.name,
        256,
        (void *)&led2_task_config,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(led2_created == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
