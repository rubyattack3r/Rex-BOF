#include <stdio.h>

#include "args.h"
#include "utest.h"

static FILE *temporary_file(void) {
#ifdef _MSC_VER
    FILE *output = NULL;
    if (tmpfile_s(&output) != 0)
        return NULL;
    return output;
#else
    return tmpfile();
#endif
}

static rex_cli_arguments parse(int argc, wchar_t **argv,
                               rex_cli_parse_status *status) {
    rex_cli_arguments arguments;
    *status = rex_cli_parse(argc, argv, &arguments);
    return arguments;
}

UTEST(args, general_help) {
    wchar_t *argv[] = {L"rex.exe", L"--help"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(2, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_EXIT, status);
    EXPECT_EQ(REX_CLI_COMMAND_HELP, arguments.command);
    EXPECT_EQ(NULL, arguments.technique);
}

UTEST(args, technique_list) {
    wchar_t *argv[] = {L"rex.exe", L"techniques"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(2, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_EXIT, status);
    EXPECT_EQ(REX_CLI_COMMAND_TECHNIQUES, arguments.command);
}

UTEST(args, technique_help) {
    wchar_t *help[] = {L"rex.exe", L"help", L"wmi-exec"};
    wchar_t *selector_help[] = {L"rex.exe", L"wmi-exec", L"-h"};
    wchar_t *preflight_help[] = {L"rex.exe", L"preflight", L"wmi-exec",
                                 L"--help"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(3, help, &status);

    EXPECT_EQ(REX_CLI_PARSE_EXIT, status);
    EXPECT_EQ(REX_CLI_COMMAND_HELP, arguments.command);
    EXPECT_NE(NULL, arguments.technique);

    arguments = parse(3, selector_help, &status);
    EXPECT_EQ(REX_CLI_PARSE_EXIT, status);
    EXPECT_EQ(REX_CLI_COMMAND_HELP, arguments.command);
    EXPECT_NE(NULL, arguments.technique);

    arguments = parse(4, preflight_help, &status);
    EXPECT_EQ(REX_CLI_PARSE_EXIT, status);
    EXPECT_EQ(REX_CLI_COMMAND_HELP, arguments.command);
    EXPECT_NE(NULL, arguments.technique);
}

UTEST(args, execution_request) {
    wchar_t *argv[] = {
        L"rex.exe",   L"wmi-exec",   L"--target",     L"server01",
        L"--command", L"whoami.exe", L"--make-token", L"operator@example.com"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(8, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_RUN, status);
    EXPECT_EQ(REX_CLI_ACTION_EXECUTE, arguments.action);
    EXPECT_NE(NULL, arguments.technique);
    EXPECT_EQ(argv[3], arguments.target);
    EXPECT_EQ(argv[5], arguments.values[0]);
    EXPECT_EQ(argv[7], arguments.principal);
    EXPECT_EQ(NULL, arguments.password);
}

UTEST(args, unknown_technique) {
    wchar_t *argv[] = {L"rex.exe", L"unknown"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(2, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_UNKNOWN_TECHNIQUE, arguments.error);
}

UTEST(args, missing_value) {
    wchar_t *argv[] = {L"rex.exe", L"wmi-exec", L"--target"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(3, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_MISSING_VALUE, arguments.error);
}

UTEST(args, duplicate_option) {
    wchar_t *argv[] = {L"rex.exe",  L"wmi-exec", L"--target",  L"one",
                       L"--target", L"two",      L"--command", L"whoami"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(8, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_DUPLICATE_OPTION, arguments.error);
}

UTEST(args, unknown_option) {
    wchar_t *argv[] = {L"rex.exe",  L"wmi-exec",  L"--target",
                       L"server01", L"--unknown", L"value"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(6, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_UNKNOWN_OPTION, arguments.error);
}

UTEST(args, missing_required_option) {
    wchar_t *argv[] = {L"rex.exe", L"wmi-exec", L"--target", L"server01"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(4, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_MISSING_REQUIRED, arguments.error);
}

UTEST(args, password_without_principal) {
    wchar_t *argv[] = {L"rex.exe",   L"wmi-exec", L"--target",   L"server01",
                       L"--command", L"whoami",   L"--password", L"secret"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(8, argv, &status);

    EXPECT_EQ(REX_CLI_PARSE_ERROR, status);
    EXPECT_EQ(REX_CLI_ERROR_PASSWORD_WITHOUT_PRINCIPAL, arguments.error);
}

UTEST(args, help_and_error_output) {
    wchar_t *argv[] = {L"rex.exe",   L"wmi-exec", L"--target",   L"server01",
                       L"--command", L"whoami",   L"--password", L"secret"};
    rex_cli_parse_status status;
    rex_cli_arguments arguments = parse(8, argv, &status);
    FILE *output = temporary_file();

    ASSERT_EQ(REX_CLI_PARSE_ERROR, status);
    ASSERT_NE(NULL, output);
    rex_cli_print_general_help(output);
    rex_cli_print_techniques(output);
    rex_cli_print_technique_help(output, rex_cli_find(L"wmi-exec"));
    rex_cli_print_error(output, &arguments);
    EXPECT_EQ(0, fclose(output));
}

UTEST_MAIN()
