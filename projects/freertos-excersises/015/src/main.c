#include <stdlib.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico_cli/pico_cli.h"

#define AUDIO_STREAM_BYTES       2048
#define DMA_AUDIO_CHUNK_BYTES    192
#define VAD_WINDOW_BYTES         640

static StreamBufferHandle_t audio_stream;
static struct repeating_timer dma_timer;
static volatile uint32_t dma_chunk_counter;
static volatile uint32_t dropped_audio_bytes;
static char cli_buffer[64];

static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static bool dma_timer_callback(struct repeating_timer *timer) {
    int16_t audio_chunk[DMA_AUDIO_CHUNK_BYTES / sizeof(int16_t)];
    BaseType_t higher_priority_task_woken = pdFALSE;

    (void)timer;

    for (size_t i = 0; i < count_of(audio_chunk); ++i) {
        uint32_t sample_number = (dma_chunk_counter * count_of(audio_chunk)) + (uint32_t)i;
        int16_t sample = (int16_t)(((sample_number % 64u) - 32) * 200);
        audio_chunk[i] = (sample_number / 400u) % 5u == 0u ? (int16_t)(sample * 2) : sample;
    }

    size_t sent = xStreamBufferSendFromISR(
        audio_stream,
        audio_chunk,
        sizeof(audio_chunk),
        &higher_priority_task_woken
    );
    if (sent < sizeof(audio_chunk)) {
        dropped_audio_bytes += (uint32_t)(sizeof(audio_chunk) - sent);
    }

    dma_chunk_counter++;
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return true;
}

static void dma_setup_task(void *task_parameters) {
    (void)task_parameters;

    configASSERT(add_repeating_timer_us(-6000, dma_timer_callback, NULL, &dma_timer));
    vTaskDelete(NULL);
}

static void vad_task(void *task_parameters) {
    uint8_t window[VAD_WINDOW_BYTES];
    uint32_t analysis_count = 0;

    (void)task_parameters;

    for (;;) {
        size_t filled = 0u;

        while (filled < sizeof(window)) {
            filled += xStreamBufferReceive(
                audio_stream,
                &window[filled],
                sizeof(window) - filled,
                portMAX_DELAY
            );
        }

        const int16_t *samples = (const int16_t *)window;
        uint32_t energy = 0u;
        for (size_t i = 0; i < (sizeof(window) / sizeof(samples[0])); ++i) {
            energy += (uint32_t)abs(samples[i]);
        }

        pico_cli_printf_color(PICO_CLI_ANSI_GREEN,
            "VAD window %lu: bytes=%u energy=%lu dropped=%lu\r\n",
            (unsigned long)analysis_count++,
            (unsigned)sizeof(window),
            (unsigned long)energy,
            (unsigned long)dropped_audio_bytes);
    }
}

static void stats_task(void *task_parameters) {
    (void)task_parameters;

    for (;;) {
        pico_cli_printf_color(PICO_CLI_ANSI_CYAN,
            "DMA chunks=%lu stream_bytes=%lu\r\n",
            (unsigned long)dma_chunk_counter,
            (unsigned long)xStreamBufferBytesAvailable(audio_stream));
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
        .prompt = "exercise-015> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Exercise 015: Stream buffer vs queue for audio\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "A synthetic DMA-complete ISR writes audio bytes to a stream buffer and the VAD task reads 20 ms windows\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n");

    audio_stream = xStreamBufferCreate(AUDIO_STREAM_BYTES, VAD_WINDOW_BYTES);
    configASSERT(audio_stream != NULL);
    configASSERT(xTaskCreate(dma_setup_task, "dma_setup", 256, NULL, tskIDLE_PRIORITY + 3, NULL) == pdPASS);
    configASSERT(xTaskCreate(vad_task, "vad", 768, NULL, tskIDLE_PRIORITY + 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(stats_task, "stats", 384, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
    configASSERT(xTaskCreate(cli_task, "cli_task", 256, (void *)&cli, tskIDLE_PRIORITY + 1, NULL) == pdPASS);

    vTaskStartScheduler();

    panic("FreeRTOS scheduler returned unexpectedly");
}
