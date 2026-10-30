#include <stdio.h>
#include <windows.h>

#include "rex.h"

typedef struct {
    const wchar_t *selector;
    const wchar_t *options[3];
    BOOL required[3];
} rex_cli_technique;

static const rex_cli_technique rex_cli_techniques[] = {
    {L"wmi-provider",
     {L"--provider-name", L"--provider-clsid", L"--remote-path"},
     {TRUE, TRUE, TRUE}},
    {L"wmi-instance-provider",
     {L"--provider-name", L"--provider-clsid", L"--remote-path"},
     {TRUE, TRUE, TRUE}},
    {L"wmi-method-provider",
     {L"--provider-name", L"--provider-clsid", L"--remote-path"},
     {TRUE, TRUE, TRUE}},
    {L"wmi-event-subscription",
     {L"--subscription-name", L"--query", L"--command"},
     {TRUE, TRUE, TRUE}},
    {L"wmi-service",
     {L"--service-name", L"--remote-path", NULL},
     {TRUE, TRUE, FALSE}},
    {L"wmi-exec", {L"--command", NULL, NULL}, {TRUE, FALSE, FALSE}},
    {L"wsman-exec",
     {L"--program", L"--arguments", L"--transport"},
     {TRUE, FALSE, FALSE}},
    {L"task-sched-exec",
     {L"--task-name", L"--program", L"--arguments"},
     {TRUE, TRUE, FALSE}},
    {L"dcom-ishell-dispatch2",
     {L"--program", L"--arguments", NULL},
     {TRUE, FALSE, FALSE}},
    {L"scm-change-service",
     {L"--service-name", L"--remote-path", NULL},
     {TRUE, TRUE, FALSE}},
    {L"scm-create-service",
     {L"--service-name", L"--remote-path", NULL},
     {TRUE, TRUE, FALSE}},
    {L"scm-command",
     {L"--service-name", L"--command", NULL},
     {TRUE, TRUE, FALSE}},
};

static BOOL rex_cli_equal(const wchar_t *left, const wchar_t *right) {
    SIZE_T index = 0;
    if (left == NULL || right == NULL)
        return FALSE;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (left[index] != right[index])
            return FALSE;
        index++;
    }
    return left[index] == right[index];
}

static const rex_cli_technique *rex_cli_find(const wchar_t *selector) {
    SIZE_T index;
    for (index = 0;
         index < sizeof(rex_cli_techniques) / sizeof(rex_cli_techniques[0]);
         index++) {
        if (rex_cli_equal(selector, rex_cli_techniques[index].selector))
            return &rex_cli_techniques[index];
    }
    return NULL;
}

static void rex_cli_usage(void) {
    fputws(L"usage: rex.exe [preflight] <technique> --target <host> "
           L"[technique options] [--make-token <DOMAIN\\user|user@domain> "
           L"[--password <password>]]\n"
           L"techniques: wmi-provider, wmi-instance-provider, "
           L"wmi-method-provider, wmi-event-subscription, wmi-service, "
           L"wmi-exec, wsman-exec, task-sched-exec, "
           L"dcom-ishell-dispatch2, scm-change-service, "
           L"scm-create-service, scm-command\n"
           L"omit --password only when an interactive no-echo prompt is "
           L"available\n",
           stderr);
}

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
    const rex_cli_technique *cli;
    const wchar_t *selector;
    const wchar_t *target = NULL;
    const wchar_t *principal = L"";
    const wchar_t *inline_password = NULL;
    const wchar_t *values[3] = {L"", L"", L""};
    BOOL seen_values[3] = {FALSE, FALSE, FALSE};
    BOOL seen_target = FALSE;
    BOOL seen_principal = FALSE;
    BOOL seen_password = FALSE;
    wchar_t password[REX_MAX_PASSWORD + 1] = {0};
    char selector_utf8[REX_MAX_SELECTOR + 1];
    char target_utf8[REX_MAX_TARGET + 1];
    rex_action action = REX_ACTION_EXECUTE;
    rex_arguments arguments = {0};
    rex_result result = {0};
    rex_technique technique = {0};
    int selector_index = 1;
    int index;
    int option_index;
    int exit_code = 2;

    (void)SetConsoleOutputCP(CP_UTF8);
    if (argc > 1 && rex_cli_equal(argv[1], L"preflight")) {
        action = REX_ACTION_PREFLIGHT;
        selector_index = 2;
    }
    if (argc <= selector_index) {
        rex_cli_usage();
        goto cleanup;
    }
    selector = argv[selector_index];
    cli = rex_cli_find(selector);
    if (cli == NULL) {
        fwprintf(stderr, L"rex.exe: unknown technique: %ls\n", selector);
        rex_cli_usage();
        goto cleanup;
    }

    for (index = selector_index + 1; index < argc; index += 2) {
        const wchar_t *option = argv[index];
        const wchar_t *value;
        if (index + 1 >= argc) {
            fwprintf(stderr, L"rex.exe: missing value for %ls\n", option);
            goto cleanup;
        }
        value = argv[index + 1];
        if (rex_cli_equal(option, L"--target")) {
            if (seen_target) {
                fputws(L"rex.exe: duplicate --target\n", stderr);
                goto cleanup;
            }
            target = value;
            seen_target = TRUE;
            continue;
        }
        if (rex_cli_equal(option, L"--make-token")) {
            if (seen_principal) {
                fputws(L"rex.exe: duplicate --make-token\n", stderr);
                goto cleanup;
            }
            principal = value;
            seen_principal = TRUE;
            continue;
        }
        if (rex_cli_equal(option, L"--password")) {
            if (seen_password) {
                fputws(L"rex.exe: duplicate --password\n", stderr);
                goto cleanup;
            }
            inline_password = value;
            seen_password = TRUE;
            continue;
        }
        for (option_index = 0; option_index < 3; option_index++) {
            if (cli->options[option_index] != NULL &&
                rex_cli_equal(option, cli->options[option_index])) {
                break;
            }
        }
        if (option_index == 3) {
            fwprintf(stderr, L"rex.exe: unknown option for %ls: %ls\n",
                     selector, option);
            goto cleanup;
        }
        if (seen_values[option_index]) {
            fwprintf(stderr, L"rex.exe: duplicate option: %ls\n", option);
            goto cleanup;
        }
        values[option_index] = value;
        seen_values[option_index] = TRUE;
    }

    if (!seen_target) {
        fputws(L"rex.exe: --target is required\n", stderr);
        goto cleanup;
    }
    for (option_index = 0; option_index < 3; option_index++) {
        if (cli->required[option_index] && !seen_values[option_index]) {
            fwprintf(stderr, L"rex.exe: %ls is required\n",
                     cli->options[option_index]);
            goto cleanup;
        }
        if (rex_wcslen(values[option_index]) > REX_MAX_COMMAND) {
            fputws(L"rex.exe: technique value is too long\n", stderr);
            goto cleanup;
        }
    }
    if (seen_password && !seen_principal) {
        fputws(L"rex.exe: --password requires --make-token\n", stderr);
        goto cleanup;
    }
    if (seen_principal) {
        SIZE_T length;
        if (seen_password) {
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
    for (option_index = 0; option_index < 3; option_index++)
        arguments.values[option_index] = values[option_index];
    result.technique = technique.selector;
    result.target = target_utf8;
    rex_invoke(&technique, &arguments, &result);
    rex_result_write(&result);
    exit_code = result.status == REX_STATUS_SUCCEEDED ? 0 : 1;

cleanup:
    rex_zero(password, sizeof(password));
    return exit_code;
}
