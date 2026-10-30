#ifndef REX_STANDALONE_ARGS_H
#define REX_STANDALONE_ARGS_H

#include <stdio.h>
#include <wchar.h>

#define REX_CLI_VALUE_COUNT 3

typedef enum {
    REX_CLI_COMMAND_RUN = 0,
    REX_CLI_COMMAND_HELP,
    REX_CLI_COMMAND_TECHNIQUES
} rex_cli_command;

typedef enum {
    REX_CLI_ACTION_EXECUTE = 0,
    REX_CLI_ACTION_PREFLIGHT
} rex_cli_action;

typedef enum {
    REX_CLI_PARSE_RUN = 0,
    REX_CLI_PARSE_EXIT,
    REX_CLI_PARSE_ERROR
} rex_cli_parse_status;

typedef enum {
    REX_CLI_ERROR_NONE = 0,
    REX_CLI_ERROR_USAGE,
    REX_CLI_ERROR_HELP_USAGE,
    REX_CLI_ERROR_TECHNIQUES_USAGE,
    REX_CLI_ERROR_UNKNOWN_TECHNIQUE,
    REX_CLI_ERROR_MISSING_VALUE,
    REX_CLI_ERROR_DUPLICATE_OPTION,
    REX_CLI_ERROR_UNKNOWN_OPTION,
    REX_CLI_ERROR_MISSING_TARGET,
    REX_CLI_ERROR_MISSING_REQUIRED,
    REX_CLI_ERROR_PASSWORD_WITHOUT_PRINCIPAL
} rex_cli_error;

typedef struct {
    const wchar_t *name;
    const wchar_t *value_name;
    const wchar_t *description;
    int required;
} rex_cli_option;

typedef struct {
    const wchar_t *selector;
    const wchar_t *summary;
    const wchar_t *syntax;
    rex_cli_option options[REX_CLI_VALUE_COUNT];
} rex_cli_technique;

typedef struct {
    rex_cli_command command;
    rex_cli_action action;
    const rex_cli_technique *technique;
    const wchar_t *target;
    const wchar_t *principal;
    const wchar_t *password;
    const wchar_t *values[REX_CLI_VALUE_COUNT];
    rex_cli_error error;
    const wchar_t *error_value;
} rex_cli_arguments;

const rex_cli_technique *rex_cli_find(const wchar_t *selector);
rex_cli_parse_status rex_cli_parse(int argc, wchar_t **argv,
                                   rex_cli_arguments *arguments);
void rex_cli_print_error(FILE *stream, const rex_cli_arguments *arguments);
void rex_cli_print_general_help(FILE *stream);
void rex_cli_print_technique_help(FILE *stream,
                                  const rex_cli_technique *technique);
void rex_cli_print_techniques(FILE *stream);

#endif
