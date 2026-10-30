#include "args.h"

#include <stddef.h>

typedef struct {
    int argc;
    wchar_t **argv;
    int index;
} rex_cli_iterator;

static const rex_cli_technique rex_cli_techniques[] = {
    {L"wmi-provider",
     L"Register and load a WMI provider DLL",
     L"wmi-provider --target <host> --provider-name <name> "
     L"--provider-clsid <{GUID}> --remote-path <provider-dll>",
     {{L"--provider-name", L"<name>", L"WMI provider name", 1},
      {L"--provider-clsid", L"<{GUID}>", L"COM class identifier", 1},
      {L"--remote-path", L"<provider-dll>", L"Target-side DLL path", 1}}},
    {L"wmi-instance-provider",
     L"Register and load a WMI instance provider DLL",
     L"wmi-instance-provider --target <host> --provider-name <name> "
     L"--provider-clsid <{GUID}> --remote-path <provider-dll>",
     {{L"--provider-name", L"<name>", L"WMI provider name", 1},
      {L"--provider-clsid", L"<{GUID}>", L"COM class identifier", 1},
      {L"--remote-path", L"<provider-dll>", L"Target-side DLL path", 1}}},
    {L"wmi-method-provider",
     L"Register and load a WMI method provider DLL",
     L"wmi-method-provider --target <host> --provider-name <name> "
     L"--provider-clsid <{GUID}> --remote-path <provider-dll>",
     {{L"--provider-name", L"<name>", L"WMI provider name", 1},
      {L"--provider-clsid", L"<{GUID}>", L"COM class identifier", 1},
      {L"--remote-path", L"<provider-dll>", L"Target-side DLL path", 1}}},
    {L"wmi-event-subscription",
     L"Create a permanent WMI command subscription",
     L"wmi-event-subscription --target <host> --subscription-name <name> "
     L"--query <WQL> --command <command-line>",
     {{L"--subscription-name", L"<name>", L"Filter and consumer name", 1},
      {L"--query", L"<WQL>", L"Event query", 1},
      {L"--command", L"<command-line>", L"Consumer command line", 1}}},
    {L"wmi-service",
     L"Change and start an existing service through WMI",
     L"wmi-service --target <host> --service-name <name> "
     L"--remote-path <service-exe>",
     {{L"--service-name", L"<name>", L"Existing service name", 1},
      {L"--remote-path", L"<service-exe>", L"Target-side executable path", 1},
      {NULL, NULL, NULL, 0}}},
    {L"wmi-exec",
     L"Create a process through WMI",
     L"wmi-exec --target <host> --command <command-line>",
     {{L"--command", L"<command-line>", L"Complete process command line", 1},
      {NULL, NULL, NULL, 0},
      {NULL, NULL, NULL, 0}}},
    {L"wsman-exec",
     L"Run a program through WSMan and wait for its exit status",
     L"wsman-exec --target <host> --program <executable> "
     L"[--arguments <arguments>] [--transport <transport>]",
     {{L"--program", L"<executable>", L"Program to run", 1},
      {L"--arguments", L"<arguments>", L"Program arguments", 0},
      {L"--transport", L"<transport>", L"HTTP or HTTPS endpoint", 0}}},
    {L"task-sched-exec",
     L"Register, run, and delete a scheduled task",
     L"task-sched-exec --target <host> --task-name <name> "
     L"--program <executable> [--arguments <arguments>]",
     {{L"--task-name", L"<name>", L"Temporary task name", 1},
      {L"--program", L"<executable>", L"Program to run", 1},
      {L"--arguments", L"<arguments>", L"Program arguments", 0}}},
    {L"dcom-ishell-dispatch2",
     L"Launch a program through an interactive Explorer session",
     L"dcom-ishell-dispatch2 --target <host> --program <executable> "
     L"[--arguments <arguments>]",
     {{L"--program", L"<executable>", L"Program to run", 1},
      {L"--arguments", L"<arguments>", L"Program arguments", 0},
      {NULL, NULL, NULL, 0}}},
    {L"scm-change-service",
     L"Change and start an existing service through SCM",
     L"scm-change-service --target <host> --service-name <name> "
     L"--remote-path <service-exe>",
     {{L"--service-name", L"<name>", L"Existing service name", 1},
      {L"--remote-path", L"<service-exe>", L"Target-side executable path", 1},
      {NULL, NULL, NULL, 0}}},
    {L"scm-create-service",
     L"Create and start a service through SCM",
     L"scm-create-service --target <host> --service-name <name> "
     L"--remote-path <service-exe>",
     {{L"--service-name", L"<name>", L"New service name", 1},
      {L"--remote-path", L"<service-exe>", L"Target-side executable path", 1},
      {NULL, NULL, NULL, 0}}},
    {L"scm-command",
     L"Run a command through a temporary SCM service",
     L"scm-command --target <host> --service-name <name> "
     L"--command <command-line>",
     {{L"--service-name", L"<name>", L"Temporary service name", 1},
      {L"--command", L"<command-line>", L"Complete service command line", 1},
      {NULL, NULL, NULL, 0}}},
};

static int rex_cli_equal(const wchar_t *left, const wchar_t *right) {
    size_t index = 0;
    if (left == NULL || right == NULL)
        return left == right;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (left[index] != right[index])
            return 0;
        index++;
    }
    return left[index] == right[index];
}

static int rex_cli_next(rex_cli_iterator *iterator, const wchar_t **value) {
    if (iterator->index >= iterator->argc)
        return 0;
    *value = iterator->argv[iterator->index++];
    return 1;
}

static rex_cli_parse_status rex_cli_fail(rex_cli_arguments *arguments,
                                         rex_cli_error error,
                                         const wchar_t *value) {
    arguments->error = error;
    arguments->error_value = value;
    return REX_CLI_PARSE_ERROR;
}

const rex_cli_technique *rex_cli_find(const wchar_t *selector) {
    size_t index;
    for (index = 0;
         index < sizeof(rex_cli_techniques) / sizeof(rex_cli_techniques[0]);
         index++) {
        if (rex_cli_equal(selector, rex_cli_techniques[index].selector))
            return &rex_cli_techniques[index];
    }
    return NULL;
}

rex_cli_parse_status rex_cli_parse(int argc, wchar_t **argv,
                                   rex_cli_arguments *arguments) {
    rex_cli_iterator iterator = {argc, argv, 1};
    rex_cli_arguments empty = {0};
    const wchar_t *token;
    int selector_index;
    int option_index;
    int seen_target = 0;
    int seen_principal = 0;
    int seen_password = 0;
    int seen_values[REX_CLI_VALUE_COUNT] = {0, 0, 0};

    if (arguments == NULL)
        return REX_CLI_PARSE_ERROR;
    *arguments = empty;
    arguments->principal = L"";
    for (option_index = 0; option_index < REX_CLI_VALUE_COUNT; option_index++)
        arguments->values[option_index] = L"";

    if (!rex_cli_next(&iterator, &token))
        return rex_cli_fail(arguments, REX_CLI_ERROR_USAGE, NULL);

    if (rex_cli_equal(token, L"help")) {
        arguments->command = REX_CLI_COMMAND_HELP;
        if (!rex_cli_next(&iterator, &token))
            return REX_CLI_PARSE_EXIT;
        arguments->technique = rex_cli_find(token);
        if (arguments->technique == NULL)
            return rex_cli_fail(arguments, REX_CLI_ERROR_UNKNOWN_TECHNIQUE,
                                token);
        if (rex_cli_next(&iterator, &token))
            return rex_cli_fail(arguments, REX_CLI_ERROR_HELP_USAGE, token);
        return REX_CLI_PARSE_EXIT;
    }
    if (rex_cli_equal(token, L"--help") || rex_cli_equal(token, L"-h")) {
        arguments->command = REX_CLI_COMMAND_HELP;
        if (rex_cli_next(&iterator, &token))
            return rex_cli_fail(arguments, REX_CLI_ERROR_HELP_USAGE, token);
        return REX_CLI_PARSE_EXIT;
    }
    if (rex_cli_equal(token, L"techniques")) {
        arguments->command = REX_CLI_COMMAND_TECHNIQUES;
        if (rex_cli_next(&iterator, &token))
            return rex_cli_fail(arguments, REX_CLI_ERROR_TECHNIQUES_USAGE,
                                token);
        return REX_CLI_PARSE_EXIT;
    }

    arguments->command = REX_CLI_COMMAND_RUN;
    arguments->action = REX_CLI_ACTION_EXECUTE;
    if (rex_cli_equal(token, L"preflight")) {
        arguments->action = REX_CLI_ACTION_PREFLIGHT;
        if (!rex_cli_next(&iterator, &token))
            return rex_cli_fail(arguments, REX_CLI_ERROR_USAGE, NULL);
    }
    arguments->technique = rex_cli_find(token);
    if (arguments->technique == NULL)
        return rex_cli_fail(arguments, REX_CLI_ERROR_UNKNOWN_TECHNIQUE, token);
    selector_index = iterator.index;
    if (selector_index < argc && selector_index + 1 == argc &&
        (rex_cli_equal(argv[selector_index], L"--help") ||
         rex_cli_equal(argv[selector_index], L"-h"))) {
        arguments->command = REX_CLI_COMMAND_HELP;
        return REX_CLI_PARSE_EXIT;
    }

    while (rex_cli_next(&iterator, &token)) {
        const wchar_t *value;
        if (!rex_cli_next(&iterator, &value))
            return rex_cli_fail(arguments, REX_CLI_ERROR_MISSING_VALUE, token);
        if (rex_cli_equal(token, L"--target")) {
            if (seen_target)
                return rex_cli_fail(arguments, REX_CLI_ERROR_DUPLICATE_OPTION,
                                    token);
            arguments->target = value;
            seen_target = 1;
            continue;
        }
        if (rex_cli_equal(token, L"--make-token")) {
            if (seen_principal)
                return rex_cli_fail(arguments, REX_CLI_ERROR_DUPLICATE_OPTION,
                                    token);
            arguments->principal = value;
            seen_principal = 1;
            continue;
        }
        if (rex_cli_equal(token, L"--password")) {
            if (seen_password)
                return rex_cli_fail(arguments, REX_CLI_ERROR_DUPLICATE_OPTION,
                                    token);
            arguments->password = value;
            seen_password = 1;
            continue;
        }
        for (option_index = 0; option_index < REX_CLI_VALUE_COUNT;
             option_index++) {
            if (rex_cli_equal(token,
                              arguments->technique->options[option_index].name))
                break;
        }
        if (option_index == REX_CLI_VALUE_COUNT)
            return rex_cli_fail(arguments, REX_CLI_ERROR_UNKNOWN_OPTION, token);
        if (seen_values[option_index])
            return rex_cli_fail(arguments, REX_CLI_ERROR_DUPLICATE_OPTION,
                                token);
        arguments->values[option_index] = value;
        seen_values[option_index] = 1;
    }

    if (!seen_target)
        return rex_cli_fail(arguments, REX_CLI_ERROR_MISSING_TARGET, NULL);
    for (option_index = 0; option_index < REX_CLI_VALUE_COUNT; option_index++) {
        if (arguments->technique->options[option_index].required &&
            !seen_values[option_index]) {
            return rex_cli_fail(
                arguments, REX_CLI_ERROR_MISSING_REQUIRED,
                arguments->technique->options[option_index].name);
        }
    }
    if (seen_password && !seen_principal) {
        return rex_cli_fail(arguments, REX_CLI_ERROR_PASSWORD_WITHOUT_PRINCIPAL,
                            L"--password");
    }
    return REX_CLI_PARSE_RUN;
}

void rex_cli_print_general_help(FILE *stream) {
    fputws(L"Usage:\n"
           L"  rex.exe <technique> --target <host> [technique options]\n"
           L"  rex.exe preflight <technique> --target <host> "
           L"[technique options]\n"
           L"  rex.exe help [technique]\n"
           L"  rex.exe techniques\n\n"
           L"Commands:\n"
           L"  preflight   Check the request without remote mutation.\n"
           L"  help        Show general or technique-specific help.\n"
           L"  techniques  List the available techniques.\n\n"
           L"Identity options:\n"
           L"  --make-token <DOMAIN\\user>  Use command-scoped network "
           L"credentials.\n"
           L"  --make-token <user@domain>   UPN syntax.\n"
           L"  --password <password>        Supply the password inline.\n"
           L"                               Omit it for a no-echo console "
           L"prompt.\n\n"
           L"Run `rex.exe techniques` or `rex.exe help <technique>` for "
           L"details.\n",
           stream);
}

void rex_cli_print_technique_help(FILE *stream,
                                  const rex_cli_technique *technique) {
    int index;
    if (technique == NULL)
        return;
    fwprintf(stream, L"Usage:\n  rex.exe [preflight] %ls\n\n%ls.\n\nOptions:\n",
             technique->syntax, technique->summary);
    fputws(L"  --target <host>                 Remote hostname or address "
           L"(required).\n",
           stream);
    for (index = 0; index < REX_CLI_VALUE_COUNT; index++) {
        const rex_cli_option *option = &technique->options[index];
        if (option->name == NULL)
            continue;
        fwprintf(stream, L"  %ls %ls\n      %ls (%ls).\n", option->name,
                 option->value_name, option->description,
                 option->required ? L"required" : L"optional");
    }
    fputws(L"  --make-token <principal>\n"
           L"      Command-scoped DOMAIN\\user or user@domain credentials "
           L"(optional).\n"
           L"  --password <password>\n"
           L"      Inline password; omit it for an interactive no-echo "
           L"prompt.\n",
           stream);
}

void rex_cli_print_techniques(FILE *stream) {
    size_t index;
    for (index = 0;
         index < sizeof(rex_cli_techniques) / sizeof(rex_cli_techniques[0]);
         index++) {
        fwprintf(stream, L"%-28ls %ls\n", rex_cli_techniques[index].selector,
                 rex_cli_techniques[index].summary);
    }
}

void rex_cli_print_error(FILE *stream, const rex_cli_arguments *arguments) {
    switch (arguments->error) {
    case REX_CLI_ERROR_USAGE:
        rex_cli_print_general_help(stream);
        return;
    case REX_CLI_ERROR_HELP_USAGE:
        fputws(L"rex.exe: usage: rex.exe help [technique]\n", stream);
        return;
    case REX_CLI_ERROR_TECHNIQUES_USAGE:
        fputws(L"rex.exe: `techniques` does not accept arguments\n", stream);
        return;
    case REX_CLI_ERROR_UNKNOWN_TECHNIQUE:
        fwprintf(stream, L"rex.exe: unknown technique: %ls\n",
                 arguments->error_value);
        fputws(L"Run `rex.exe techniques` to list valid selectors.\n", stream);
        return;
    case REX_CLI_ERROR_MISSING_VALUE:
        fwprintf(stream, L"rex.exe: missing value for %ls\n",
                 arguments->error_value);
        return;
    case REX_CLI_ERROR_DUPLICATE_OPTION:
        fwprintf(stream, L"rex.exe: duplicate option: %ls\n",
                 arguments->error_value);
        return;
    case REX_CLI_ERROR_UNKNOWN_OPTION:
        fwprintf(stream, L"rex.exe: unknown option for %ls: %ls\n",
                 arguments->technique->selector, arguments->error_value);
        return;
    case REX_CLI_ERROR_MISSING_TARGET:
        fputws(L"rex.exe: --target is required\n", stream);
        return;
    case REX_CLI_ERROR_MISSING_REQUIRED:
        fwprintf(stream, L"rex.exe: %ls is required\n", arguments->error_value);
        return;
    case REX_CLI_ERROR_PASSWORD_WITHOUT_PRINCIPAL:
        fputws(L"rex.exe: --password requires --make-token\n", stream);
        return;
    case REX_CLI_ERROR_NONE:
        return;
    }
}
