#include <windows.h>
#include <winsvc.h>

#include "rex.h"
#include "service.h"
#include "technique.h"

REX_DECLARE_TECHNIQUE(rex_scm_change);

const rex_technique rex_technique_descriptor = {
    "SCMChangeService", "scm_service_snapshot", "scm_start_service",
    REX_DISPOSITION_DISPATCHED,
    rex_scm_change_decode, rex_scm_change_preflight,
    rex_scm_change_execute, rex_scm_change_release,
};

static HRESULT rex_scm_change_decode(const rex_arguments *arguments,
                              rex_request *request) {
    if (rex_wcslen(arguments->values[0]) == 0 ||
        rex_wcslen(arguments->values[1]) == 0 ||
        rex_wcslen(arguments->values[2]) != 0) {
        return E_INVALIDARG;
    }
    request->config.service.name = arguments->values[0];
    request->config.service.binary_path = arguments->values[1];
    return S_OK;
}

static HRESULT rex_scm_change_preflight(const rex_request *request,
                                 rex_state *state) {
    rex_service_snapshot *snapshot = NULL;
    HRESULT result = rex_service_snapshot_open(
        request->target, request->config.service.name, SERVICE_START, &snapshot);
    if (SUCCEEDED(result)) {
        state->private_data = snapshot;
    }
    return result;
}

static HRESULT rex_scm_change_execute(const rex_request *request,
                               rex_state *state,
                               rex_observation *observation) {
    rex_service_snapshot *snapshot = (rex_service_snapshot *)state->private_data;
    HRESULT result;
    if (snapshot == NULL) {
        return E_UNEXPECTED;
    }
    result = rex_service_change_native(snapshot, request->config.service.binary_path);
    if (SUCCEEDED(result)) {
        observation->recovery.kind = REX_RECOVERY_RESTORE_SERVICE;
        observation->recovery.value.service.name = request->config.service.name;
        observation->recovery.value.service.original_binary_path =
            rex_service_original_binary_path(snapshot);
        observation->recovery.value.service.applied_binary_path =
            request->config.service.binary_path;
        observation->recovery.value.service.original_start_type =
            rex_service_original_start_type(snapshot);
        result = rex_service_start_native(snapshot);
    }
    return result;
}

static void rex_scm_change_release(rex_state *state) {
    rex_service_snapshot_close((rex_service_snapshot *)state->private_data);
    state->private_data = NULL;
}
