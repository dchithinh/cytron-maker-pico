#ifndef PICO_CLI_H
#define PICO_CLI_H

#include <stddef.h>

#define PICO_CLI_ANSI_RESET   "\x1b[0m"
#define PICO_CLI_ANSI_BOLD    "\x1b[1m"
#define PICO_CLI_ANSI_DIM     "\x1b[2m"
#define PICO_CLI_ANSI_RED     "\x1b[31m"
#define PICO_CLI_ANSI_GREEN   "\x1b[32m"
#define PICO_CLI_ANSI_YELLOW  "\x1b[33m"
#define PICO_CLI_ANSI_BLUE    "\x1b[34m"
#define PICO_CLI_ANSI_MAGENTA "\x1b[35m"
#define PICO_CLI_ANSI_CYAN    "\x1b[36m"

typedef struct pico_cli pico_cli_t;

typedef int (*pico_cli_command_handler_t)(pico_cli_t *cli, int argc, char **argv, void *context);

typedef struct {
    const char *name;
    const char *help;
    pico_cli_command_handler_t handler;
} pico_cli_command_t;

struct pico_cli {
    const char *prompt;
    char *buffer;
    size_t buffer_size;
    const pico_cli_command_t *commands;
    size_t command_count;
    void *context;
    size_t length;
};

void pico_cli_init(pico_cli_t *cli);
void pico_cli_poll(pico_cli_t *cli);
void pico_cli_printf_color(const char *color, const char *format, ...);
void pico_cli_print_prompt(const pico_cli_t *cli);
void pico_cli_print_help(const pico_cli_t *cli);
const pico_cli_command_t *pico_cli_find_command(const pico_cli_t *cli, const char *name);
int pico_cli_execute_line(pico_cli_t *cli, char *line);
int pico_cli_help_command(pico_cli_t *cli, int argc, char **argv, void *context);
int pico_cli_boot_command(pico_cli_t *cli, int argc, char **argv, void *context);

#endif
