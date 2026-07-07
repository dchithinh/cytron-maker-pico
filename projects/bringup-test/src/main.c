#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico_cli/pico_cli.h"
#include "ws2812.pio.h"

#define NEOPIXEL_IS_RGBW false

static const uint button_pins[] = {BOARD_BUTTON1_PIN, BOARD_BUTTON2_PIN, BOARD_BUTTON3_PIN};
static const uint indicator_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 26, 27};
static char cli_buffer[64];
static const pico_cli_command_t cli_commands[] = {
    {.name = "help", .help = "List available commands", .handler = pico_cli_help_command},
    {.name = "boot", .help = "Jump to UF2 bootloader mode", .handler = pico_cli_boot_command},
};

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 8u) | ((uint32_t)g << 16u) | (uint32_t)b;
}

static void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static void set_rgb(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b) {
    put_pixel(pio, sm, urgb_u32(r, g, b));
}

static void init_neopixel(PIO pio, uint sm, uint offset, uint pin, float frequency_hz, bool rgbw) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    pio_sm_config config = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&config, pin);
    sm_config_set_out_shift(&config, false, true, rgbw ? 32 : 24);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

    int cycles_per_bit = ws2812_T1 + ws2812_T2 + ws2812_T3;
    float divider = (float)clock_get_hz(clk_sys) / (frequency_hz * cycles_per_bit);
    sm_config_set_clkdiv(&config, divider);

    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_enabled(pio, sm, true);
}

static void init_indicator_leds(void) {
    for (size_t i = 0; i < count_of(indicator_pins); ++i) {
        gpio_init(indicator_pins[i]);
        gpio_set_dir(indicator_pins[i], GPIO_OUT);
        gpio_put(indicator_pins[i], false);
    }
}

static void init_buttons(void) {
    for (size_t i = 0; i < count_of(button_pins); ++i) {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }
}

static void init_buzzer(void) {
    gpio_set_function(BOARD_BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BOARD_BUZZER_PIN);
    pwm_set_enabled(slice_num, true);
}

static void play_tone(uint gpio, uint32_t frequency_hz, uint32_t duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);

    if (frequency_hz == 0) {
        pwm_set_chan_level(slice_num, channel, 0);
        sleep_ms(duration_ms);
        return;
    }

    const uint32_t clock_divider = 100;
    pwm_set_clkdiv_int_frac(slice_num, clock_divider, 0);

    uint32_t wrap = (125000000u / clock_divider / frequency_hz) - 1u;
    if (wrap > 65535u) {
        wrap = 65535u;
    }

    pwm_set_wrap(slice_num, (uint16_t)wrap);
    pwm_set_chan_level(slice_num, channel, wrap / 2u);
    sleep_ms(duration_ms);
    pwm_set_chan_level(slice_num, channel, 0);
}

static void startup_chime(void) {
    const uint16_t notes[] = {523, 659, 784};
    for (size_t i = 0; i < count_of(notes); ++i) {
        play_tone(BOARD_BUZZER_PIN, notes[i], 100);
        sleep_ms(30);
    }
}

static uint32_t indicator_mask_for_buttons(void) {
    uint32_t mask = 0;

    for (size_t i = 0; i < count_of(button_pins); ++i) {
        if (!gpio_get(button_pins[i])) {
            mask |= 1u << i;
        }
    }

    return mask;
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500);

    init_indicator_leds();
    init_buttons();
    init_buzzer();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    init_neopixel(pio, sm, offset, BOARD_NEOPIXEL_PIN, 800000.0f, NEOPIXEL_IS_RGBW);

    pico_cli_t cli = {
        .prompt = "maker-pico> ",
        .buffer = cli_buffer,
        .buffer_size = sizeof(cli_buffer),
        .commands = cli_commands,
        .command_count = count_of(cli_commands),
        .context = NULL,
    };
    pico_cli_init(&cli);

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Maker Pi Pico hardware smoke test\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Buttons: ");
    printf("GP20, GP21, GP22 (active low)\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Buzzer: ");
    printf("GP18, ");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "NeoPixel: ");
    printf("GP28\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_CYAN, "Indicator LEDs under test: ");
    printf("GP0-GP9, GP26-GP27\r\n");
    pico_cli_printf_color(PICO_CLI_ANSI_DIM, "USB CLI: open the Pico USB serial port and type: help or boot\r\n\r\n");

    startup_chime();

    size_t chase_index = 0;
    uint32_t last_button_mask = UINT32_MAX;

    while (true) {
        pico_cli_poll(&cli);

        uint32_t button_mask = indicator_mask_for_buttons();

        for (size_t i = 0; i < count_of(indicator_pins); ++i) {
            bool active = (i == chase_index);
            gpio_put(indicator_pins[i], active);
        }

        switch (button_mask) {
            case 0:
                set_rgb(pio, sm, 0x00, 0x10, 0x00);
                break;
            case 1:
                set_rgb(pio, sm, 0x20, 0x00, 0x00);
                break;
            case 2:
                set_rgb(pio, sm, 0x00, 0x00, 0x20);
                break;
            case 4:
                set_rgb(pio, sm, 0x20, 0x20, 0x00);
                break;
            default:
                set_rgb(pio, sm, 0x10, 0x00, 0x10);
                break;
        }

        if (button_mask != last_button_mask) {
            pico_cli_printf_color(PICO_CLI_ANSI_YELLOW, "Buttons state: ");
            printf("GP20=%d GP21=%d GP22=%d\r\n",
                (button_mask & 0x1u) ? 1 : 0,
                (button_mask & 0x2u) ? 1 : 0,
                (button_mask & 0x4u) ? 1 : 0);

            if (button_mask & 0x1u) {
                play_tone(BOARD_BUZZER_PIN, 523, 60);
            }
            if (button_mask & 0x2u) {
                play_tone(BOARD_BUZZER_PIN, 659, 60);
            }
            if (button_mask & 0x4u) {
                play_tone(BOARD_BUZZER_PIN, 784, 60);
            }

            last_button_mask = button_mask;
        }

        chase_index = (chase_index + 1) % count_of(indicator_pins);
        sleep_ms(120);
    }
}
