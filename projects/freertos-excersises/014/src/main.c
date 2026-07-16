#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static volatile uint32_t sleep_entry_count;
static volatile uint32_t sleep_exit_count;
static volatile uint32_t last_expected_idle_ticks;

void exercise_014_pre_sleep(uint32_t expected_idle_ticks) {
    sleep_entry_count++;
    last_expected_idle_ticks = expected_idle_ticks;
    gpio_put(BOARD_LED_PIN, false);
}

void exercise_014_post_sleep(uint32_t expected_idle_ticks) {
    (void)expected_idle_ticks;
    sleep_exit_count++;
    gpio_put(BOARD_LED_PIN, true);
}

static void sample_transmit_task(void *task_parameters) {
    (void)task_parameters;
    uint32_t cycle = 0;

    for (;;) {
        printf("Exercise 014 cycle %lu: wake, sample, transmit\r\n", (unsigned long)cycle);
        gpio_put(BOARD_LED_PIN, true);
        vTaskDelay(pdMS_TO_TICKS(75));
        printf("Cycle %lu: pre_sleep=%lu post_sleep=%lu last_idle_ticks=%lu\r\n",
            (unsigned long)cycle,
            (unsigned long)sleep_entry_count,
            (unsigned long)sleep_exit_count,
            (unsigned long)last_expected_idle_ticks);

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(60000));
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

    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_put(BOARD_LED_PIN, true);

    printf("Exercise 014: Tickless-idle sensor node\r\n");
    printf("Tickless idle is enabled and the single task wakes every 60 seconds\r\n");
    printf("The LED is driven low in the pre-sleep hook and high again in the post-sleep hook\r\n");

    configASSERT(xTaskCreate(sample_transmit_task, "sample_tx", 512, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
