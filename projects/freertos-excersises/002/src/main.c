#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "bsp/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico_cli/pico_cli.h"

static SemaphoreHandle_t button_semaphore;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static void button_irq_handler(uint gpio, uint32_t events) {
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (gpio == BOARD_BUTTON1_PIN && (events & GPIO_IRQ_EDGE_FALL) != 0u) {
        xSemaphoreGiveFromISR(button_semaphore, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static void input_task(void *task_parameters) {
    (void)task_parameters;
    bool led_level = false;

    gpio_init(BOARD_LED_PIN);
    gpio_set_dir(BOARD_LED_PIN, GPIO_OUT);
    gpio_put(BOARD_LED_PIN, led_level);

    for (;;) {
        xSemaphoreTake(button_semaphore, portMAX_DELAY);
        led_level = !led_level;
        gpio_put(BOARD_LED_PIN, led_level);
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW, "Button press handled: LED is now %s\r\n",
            led_level ? "ON" : "OFF");
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
        .prompt = "exercise-002> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 002: Button-controlled LED via ISR\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Button: ");
    printf("GP%u (active low interrupt)\r\n", BOARD_BUTTON1_PIN);
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "LED: ");
    printf("GP%u toggled by input task\r\n", BOARD_LED_PIN);
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    gpio_init(BOARD_BUTTON1_PIN);
    gpio_set_dir(BOARD_BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BOARD_BUTTON1_PIN);

    button_semaphore = xSemaphoreCreateBinary();
    configASSERT(button_semaphore != NULL);

    gpio_set_irq_enabled_with_callback(
        BOARD_BUTTON1_PIN,
        GPIO_IRQ_EDGE_FALL,
        true,
        &button_irq_handler
    );

    BaseType_t input_created = xTaskCreate(
        input_task,
        "input_task",
        256,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
    configASSERT(input_created == pdPASS);

    BaseType_t cli_created = xTaskCreate(
        cli_task,
        "cli_task",
        256,
        (void *)&cli,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(cli_created == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
