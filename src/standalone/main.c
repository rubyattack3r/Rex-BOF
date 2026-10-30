#include <stdio.h>
#include <windows.h>

#include "rex.h"
#include "args.h"

static BOOL rex_cli_utf8(const wchar_t *source, char *destination,
                         int capacity) {
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source, -1,
                               destination, capacity, NULL, NULL) > 0;
}

static BOOL rex_cli_password(wchar_t password[REX_MAX_PASSWORD + 1]) {
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    wchar_t entered[REX_MAX_PASSWORD + 3];
    DWORD original_mode;
    DWORD mode;
    DWORD read;

    if (input == NULL || input == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(input, &original_mode)) {
        fputws(L"rex.exe: --password is required when stdin is not a console\n",
               stderr);
        return FALSE;
    }
    mode = original_mode & ~ENABLE_ECHO_INPUT;
    if (!SetConsoleMode(input, mode))
        return FALSE;
    fputws(L"Password: ", stderr);
    (void)fflush(stderr);
    if (!ReadConsoleW(input, entered, REX_MAX_PASSWORD + 2, &read, NULL))
        read = 0;
    (void)SetConsoleMode(input, original_mode);
    fputws(L"\n", stderr);
    while (read > 0 &&
           (entered[read - 1] == L'\r' || entered[read - 1] == L'\n')) {
        read--;
    }
    if (read == 0 || read > REX_MAX_PASSWORD) {
        rex_zero(entered, sizeof(entered));
        return FALSE;
    }
    rex_copy(password, entered, read * sizeof(wchar_t));
    password[read] = L'\0';
    rex_zero(entered, sizeof(entered));
    return TRUE;
}

int wmain(int argc, wchar_t **argv) {
    rex_cli_arguments cli;
    rex_cli_parse_status parse_status;
    const wchar_t *selector;
    const wchar_t *target;
    const wchar_t *principal;
    const wchar_t *inline_password;
    wchar_t password[REX_MAX_PASSWORD + 1] = {0};
    char selector_utf8[REX_MAX_SELECTOR + 1];
    char target_utf8[REX_MAX_TARGET + 1];
    rex_action action;
    rex_arguments arguments = {0};
    rex_result result = {0};
    rex_technique technique = {0};
    int option_index;
    int exit_code = 2;

    (void)SetConsoleOutputCP(CP_UTF8);
    parse_status = rex_cli_parse(argc, argv, &cli);
    if (parse_status == REX_CLI_PARSE_EXIT) {
        if (cli.command == REX_CLI_COMMAND_TECHNIQUES)
            rex_cli_print_techniques(stdout);
        else if (cli.technique != NULL)
            rex_cli_print_technique_help(stdout, cli.technique);
        else
            rex_cli_print_general_help(stdout);
        exit_code = 0;
        goto cleanup;
    }
    if (parse_status == REX_CLI_PARSE_ERROR) {
        rex_cli_print_error(stderr, &cli);
        goto cleanup;
    }

    selector = cli.technique->selector;
    target = cli.target;
    principal = cli.principal;
    inline_password = cli.password;
    action = cli.action == REX_CLI_ACTION_PREFLIGHT ? REX_ACTION_PREFLIGHT
                                                    : REX_ACTION_EXECUTE;
    for (option_index = 0; option_index < REX_CLI_VALUE_COUNT; option_index++) {
        if (rex_wcslen(cli.values[option_index]) > REX_MAX_COMMAND) {
            fputws(L"rex.exe: technique value is too long\n", stderr);
            goto cleanup;
        }
    }
    if (principal[0] != L'\0') {
        SIZE_T length;
        if (inline_password != NULL) {
            length = rex_wcslen(inline_password);
            if (length == 0 || length > REX_MAX_PASSWORD) {
                fputws(L"rex.exe: invalid password length\n", stderr);
                goto cleanup;
            }
            rex_copy(password, inline_password, (length + 1) * sizeof(wchar_t));
            rex_zero((void *)inline_password, (length + 1) * sizeof(wchar_t));
        } else if (!rex_cli_password(password)) {
            fputws(L"rex.exe: could not read password\n", stderr);
            goto cleanup;
        }
    }

    if (!rex_cli_utf8(selector, selector_utf8, sizeof(selector_utf8)) ||
        !rex_cli_utf8(target, target_utf8, sizeof(target_utf8)) ||
        !rex_valid_selector(selector_utf8) || !rex_valid_target(target_utf8) ||
        !rex_find_technique(selector_utf8, &technique)) {
        fputws(L"rex.exe: invalid selector or target\n", stderr);
        goto cleanup;
    }

    arguments.action = action;
    arguments.target = target_utf8;
    arguments.selector = selector_utf8;
    arguments.identity.principal = principal;
    arguments.identity.password = password;
    for (option_index = 0; option_index < REX_CLI_VALUE_COUNT; option_index++)
        arguments.values[option_index] = cli.values[option_index];
    result.technique = technique.selector;
    result.target = target_utf8;
    rex_invoke(&technique, &arguments, &result);
    rex_result_write(&result);
    exit_code = result.status == REX_STATUS_SUCCEEDED ? 0 : 1;

cleanup:
    rex_zero(password, sizeof(password));
    return exit_code;
}
