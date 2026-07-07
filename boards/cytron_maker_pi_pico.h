/*
 * Board definition for the Cytron Maker Pi Pico.
 *
 * Use with the Pico SDK via:
 *   -DPICO_BOARD=cytron_maker_pi_pico
 *   -DPICO_BOARD_HEADER_DIRS=/path/to/repo/boards
 *
 * This file should remain preprocessor-only because the SDK may include it
 * from assembler sources.
 */

#ifndef _BOARDS_CYTRON_MAKER_PI_PICO_H
#define _BOARDS_CYTRON_MAKER_PI_PICO_H

#define CYTRON_MAKER_PI_PICO

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 28
#endif

// --- I2C ---
// Default to Grove-compatible I2C0 on GP4/GP5.
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
// Default to the onboard MicroSD wiring on SPI1.
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 10
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 11
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 12
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 15
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1
#endif

#ifndef PICO_SMPS_MODE_PIN
#define PICO_SMPS_MODE_PIN 23
#endif

#ifndef PICO_VBUS_PIN
#define PICO_VBUS_PIN 24
#endif

#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN 29
#endif

// --- Onboard peripherals ---
#define CYTRON_BUZZER_PIN 18
#define CYTRON_AUDIO_LEFT_PIN 18
#define CYTRON_AUDIO_RIGHT_PIN 19

#define CYTRON_BUTTON1_PIN 20
#define CYTRON_BUTTON2_PIN 21
#define CYTRON_BUTTON3_PIN 22

#define CYTRON_WS2812_PIN 28

#define CYTRON_SD_SPI 1
#define CYTRON_SD_SCK_PIN 10
#define CYTRON_SD_MOSI_PIN 11
#define CYTRON_SD_MISO_PIN 12
#define CYTRON_SD_CS_PIN 15

#define CYTRON_ESP_UART 0
#define CYTRON_ESP_TX_PIN 16
#define CYTRON_ESP_RX_PIN 17

// Grove ports, top to bottom as printed on the board.
#define CYTRON_GROVE1_PIN1 0
#define CYTRON_GROVE1_PIN2 1
#define CYTRON_GROVE2_PIN1 2
#define CYTRON_GROVE2_PIN2 3
#define CYTRON_GROVE3_PIN1 4
#define CYTRON_GROVE3_PIN2 5
#define CYTRON_GROVE4_PIN1 6
#define CYTRON_GROVE4_PIN2 7
#define CYTRON_GROVE5_PIN1 8
#define CYTRON_GROVE5_PIN2 9
#define CYTRON_GROVE6_PIN1 26
#define CYTRON_GROVE6_PIN2 27

#endif
