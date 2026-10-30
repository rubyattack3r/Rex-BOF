#include <stdio.h>

#include "args.h"

static unsigned int failures;

static rex_cli_arguments parse(int argc, wchar_t **argv,
                               rex_cli_parse_status expected) {
    rex_cli_arguments arguments;
    rex_cli_parse_status status = rex_cli_parse(argc, argv, &arguments);
    if (status != expected)
        failures++;
    return arguments;
}

int main(void) {
    wchar_t *general[] = {L"rex.exe", L"--help"};
    wchar_t *techniques[] = {L"rex.exe", L"techniques"};
    wchar_t *help[] = {L"rex.exe", L"help", L"wmi-exec"};
    wchar_t *selector_help[] = {L"rex.exe", L"wmi-exec", L"-h"};
    wchar_t *preflight_help[] = {L"rex.exe", L"preflight", L"wmi-exec",
                                 L"--help"};
    wchar_t *run[] = {
        L"rex.exe",   L"wmi-exec",   L"--target",     L"server01",
        L"--command", L"whoami.exe", L"--make-token", L"operator@example.com"};
    wchar_t *unknown[] = {L"rex.exe", L"unknown"};
    wchar_t *missing_value[] = {L"rex.exe", L"wmi-exec", L"--target"};
    wchar_t *duplicate[] = {L"rex.exe",  L"wmi-exec", L"--target",  L"one",
                            L"--target", L"two",      L"--command", L"whoami"};
    wchar_t *unknown_option[] = {L"rex.exe",  L"wmi-exec",  L"--target",
                                 L"server01", L"--unknown", L"value"};
    wchar_t *missing_required[] = {L"rex.exe", L"wmi-exec", L"--target",
                                   L"server01"};
    wchar_t *password_only[] = {L"rex.exe",    L"wmi-exec",  L"--target",
                                L"server01",   L"--command", L"whoami",
                                L"--password", L"secret"};
    rex_cli_arguments arguments;
    FILE *output;

    arguments = parse(2, general, REX_CLI_PARSE_EXIT);
    if (arguments.command != REX_CLI_COMMAND_HELP ||
        arguments.technique != NULL)
        failures++;
    arguments = parse(2, techniques, REX_CLI_PARSE_EXIT);
    if (arguments.command != REX_CLI_COMMAND_TECHNIQUES)
        failures++;
    arguments = parse(3, help, REX_CLI_PARSE_EXIT);
    if (arguments.command != REX_CLI_COMMAND_HELP ||
        arguments.technique == NULL)
        failures++;
    arguments = parse(3, selector_help, REX_CLI_PARSE_EXIT);
    if (arguments.command != REX_CLI_COMMAND_HELP ||
        arguments.technique == NULL)
        failures++;
    arguments = parse(4, preflight_help, REX_CLI_PARSE_EXIT);
    if (arguments.command != REX_CLI_COMMAND_HELP ||
        arguments.technique == NULL)
        failures++;

    arguments = parse(8, run, REX_CLI_PARSE_RUN);
    if (arguments.action != REX_CLI_ACTION_EXECUTE ||
        arguments.technique == NULL || arguments.target != run[3] ||
        arguments.values[0] != run[5] || arguments.principal != run[7] ||
        arguments.password != NULL)
        failures++;

    arguments = parse(2, unknown, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_UNKNOWN_TECHNIQUE)
        failures++;
    arguments = parse(3, missing_value, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_MISSING_VALUE)
        failures++;
    arguments = parse(8, duplicate, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_DUPLICATE_OPTION)
        failures++;
    arguments = parse(6, unknown_option, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_UNKNOWN_OPTION)
        failures++;
    arguments = parse(4, missing_required, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_MISSING_REQUIRED)
        failures++;
    arguments = parse(8, password_only, REX_CLI_PARSE_ERROR);
    if (arguments.error != REX_CLI_ERROR_PASSWORD_WITHOUT_PRINCIPAL)
        failures++;

    output = tmpfile();
    if (output == NULL) {
        failures++;
    } else {
        rex_cli_print_general_help(output);
        rex_cli_print_techniques(output);
        rex_cli_print_technique_help(output, rex_cli_find(L"wmi-exec"));
        rex_cli_print_error(output, &arguments);
        fclose(output);
    }

    if (failures != 0) {
        fprintf(stderr, "argument tests failed: %u\n", failures);
        return 1;
    }
    puts("argument tests passed");
    return 0;
}
