#include <windows.h>

#include "rex.h"
#include "service.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_scm_command, "scm-command",
                     "scm_service_name_available", "scm_start_service",
                     REX_DISPOSITION_DISPATCHED);

static HRESULT rex_scm_command_decode(const rex_arguments *arguments,
                                      rex_request *request) {
    if (arguments->values[0][0] == L'\0' || arguments->values[1][0] == L'\0' ||
        arguments->values[2][0] != L'\0') {
        return E_INVALIDARG;
    }
    request->config.service.name = arguments->values[0];
    request->config.service.binary_path = arguments->values[1];
    return S_OK;
}

static HRESULT rex_scm_command_preflight(const rex_request *request,
                                         rex_state *state) {
    rex_created_service *created = NULL;
    HRESULT result = rex_service_create_preflight(
        request->target, request->config.service.name, &created);
    if (SUCCEEDED(result))
        state->private_data = created;
    return result;
}

static HRESULT rex_scm_command_execute(const rex_request *request,
                                       rex_state *state,
                                       rex_observation *observation) {
    rex_created_service *created = (rex_created_service *)state->private_data;
    HRESULT result =
        rex_service_create(created, request->config.service.binary_path);
    if (SUCCEEDED(result)) {
        observation->recovery.kind = REX_RECOVERY_DELETE_SERVICE;
        observation->recovery.value.created_service.name =
            request->config.service.name;
        observation->recovery.value.created_service.binary_path = NULL;
        result = rex_service_created_start_command(created);
    }
    return result;
}

static void rex_scm_command_release(rex_state *state) {
    rex_service_created_close((rex_created_service *)state->private_data);
    state->private_data = NULL;
}
