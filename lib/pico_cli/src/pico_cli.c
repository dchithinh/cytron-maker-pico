#include "pico_cli/pico_cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#define PICO_CLI_MAX_ARGS 8

void pico_cli_printf_color(const char *color, const char *format, ...) {
    va_list args;

    if (color != NULL) {
        printf("%s", color);
    }

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    if (color != NULL) {
        printf(PICO_CLI_ANSI_RESET);
    }
}

void pico_cli_print_prompt(const pico_cli_t *cli) {
    if (cli->prompt != NULL) {
        pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_CYAN, "%s", cli->prompt);
    }
}

const pico_cli_command_t *pico_cli_find_command(const pico_cli_t *cli, const char *name) {
    if (cli == NULL || name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < cli->command_count; ++i) {
        const pico_cli_command_t *command = &cli->commands[i];

        if (strcmp(name, command->name) == 0) {
            return command;
        }
    }

    return NULL;
}

void pico_cli_print_help(const pico_cli_t *cli) {
    if (cli == NULL) {
        return;
    }

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_GREEN, "Available commands:\r\n");
    for (size_t i = 0; i < cli->command_count; ++i) {
        const pico_cli_command_t *command = &cli->commands[i];
        pico_cli_printf_color(PICO_CLI_ANSI_YELLOW, "  %s", command->name);

        if (command->help != NULL && command->help[0] != '\0') {
            pico_cli_printf_color(PICO_CLI_ANSI_DIM, " - %s", command->help);
        }

        printf("\r\n");
    }
}

int pico_cli_execute_line(pico_cli_t *cli, char *line) {
    char *argv[PICO_CLI_MAX_ARGS];
    int argc = 0;
    char *cursor = line;

    if (cli == NULL || line == NULL) {
        return -1;
    }

    while (*cursor != '\0' && argc < PICO_CLI_MAX_ARGS) {
        while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
            ++cursor;
        }

        if (*cursor == '\0') {
            break;
        }

        argv[argc++] = cursor;

        while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
            ++cursor;
        }

        if (*cursor == '\0') {
            break;
        }

        *cursor++ = '\0';
    }

    if (argc == 0) {
        return 0;
    }

    const pico_cli_command_t *command = pico_cli_find_command(cli, argv[0]);
    if (command == NULL) {
        pico_cli_printf_color(PICO_CLI_ANSI_RED, "Unknown command: %s\r\n", argv[0]);
        pico_cli_printf_color(PICO_CLI_ANSI_DIM, "Type 'help' to list commands.\r\n");
        return -1;
    }

    return command->handler(cli, argc, argv, cli->context);
}

void pico_cli_init(pico_cli_t *cli) {
    if (cli == NULL || cli->buffer == NULL || cli->buffer_size < 2u) {
        return;
    }

    cli->length = 0;
    cli->buffer[0] = '\0';
    pico_cli_print_prompt(cli);
}

void pico_cli_poll(pico_cli_t *cli) {
    int ch;

    if (cli == NULL || cli->buffer == NULL || cli->buffer_size < 2u) {
        return;
    }

    while ((ch = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        if (ch == '\r' || ch == '\n') {
            printf("\r\n");
            cli->buffer[cli->length] = '\0';
            pico_cli_execute_line(cli, cli->buffer);
            cli->length = 0;
            cli->buffer[0] = '\0';
            pico_cli_print_prompt(cli);
            continue;
        }

        if (ch == '\b' || ch == 0x7f) {
            if (cli->length > 0u) {
                --cli->length;
                cli->buffer[cli->length] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (!isprint((unsigned char)ch)) {
            continue;
        }

        if (cli->length + 1u >= cli->buffer_size) {
            pico_cli_printf_color(PICO_CLI_ANSI_RED, "\r\nCommand too long\r\n");
            cli->length = 0;
            cli->buffer[0] = '\0';
            pico_cli_print_prompt(cli);
            continue;
        }

        cli->buffer[cli->length++] = (char)ch;
        cli->buffer[cli->length] = '\0';
        putchar_raw(ch);
    }
}

int pico_cli_help_command(pico_cli_t *cli, int argc, char **argv, void *context) {
    (void)argc;
    (void)argv;
    (void)context;

    pico_cli_print_help(cli);
    return 0;
}

int pico_cli_boot_command(pico_cli_t *cli, int argc, char **argv, void *context) {
    (void)cli;
    (void)argc;
    (void)argv;
    (void)context;

    pico_cli_printf_color(PICO_CLI_ANSI_BOLD PICO_CLI_ANSI_MAGENTA, "Rebooting into UF2 bootloader...\r\n");
    sleep_ms(10);
    reset_usb_boot(0u, 0u);
    return 0;
}
