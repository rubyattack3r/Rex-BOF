#include "rex.h"

static void rex_lifecycle_release(const rex_technique *technique,
                                  rex_state *state) {
    if (state->lifecycle == REX_LIFECYCLE_RELEASED ||
        state->lifecycle == REX_LIFECYCLE_RELEASING) {
        return;
    }
    state->lifecycle = REX_LIFECYCLE_RELEASING;
    technique->release(state);
    state->lifecycle = REX_LIFECYCLE_RELEASED;
}

static void rex_lifecycle_fail(rex_result *result, rex_phase phase,
                               HRESULT error) {
    result->status = REX_STATUS_FAILED;
    result->phase = phase;
    result->error = error;
}

void rex_lifecycle_run(const rex_technique *technique,
                       const rex_arguments *arguments, rex_result *result) {
    rex_observation observation = {0};
    rex_request request = {0};
    rex_state state = {0};
    HRESULT status;

    result->technique = technique->selector;
    result->target = arguments->target;
    request.target = arguments->target;
    request.selector = arguments->selector;

    status = technique->decode(arguments, &request);
    if (FAILED(status)) {
        rex_lifecycle_release(technique, &state);
        rex_lifecycle_fail(result, REX_PHASE_INPUT, status);
        return;
    }
    state.lifecycle = REX_LIFECYCLE_DECODED;
    state.lifecycle = REX_LIFECYCLE_PREFLIGHTING;
    status = technique->preflight(&request, &state);
    if (FAILED(status)) {
        rex_lifecycle_release(technique, &state);
        rex_lifecycle_fail(result, REX_PHASE_PREFLIGHT, status);
        return;
    }
    state.lifecycle = REX_LIFECYCLE_READY;
    if (arguments->action == REX_ACTION_PREFLIGHT) {
        rex_lifecycle_release(technique, &state);
        result->status = REX_STATUS_SUCCEEDED;
        result->disposition = REX_DISPOSITION_VALIDATED;
        result->evidence_source = technique->preflight_evidence_source;
        return;
    }

    state.lifecycle = REX_LIFECYCLE_EXECUTING;
    status = technique->execute(&request, &state, &observation);
    result->observation = observation;
    if (FAILED(status)) {
        rex_lifecycle_release(technique, &state);
        rex_lifecycle_fail(result, REX_PHASE_EXECUTION, status);
        return;
    }
    state.lifecycle = REX_LIFECYCLE_EXECUTED;
    rex_lifecycle_release(technique, &state);

    result->status = REX_STATUS_SUCCEEDED;
    result->disposition = technique->success_disposition;
    result->evidence_source = technique->execution_evidence_source;
}
