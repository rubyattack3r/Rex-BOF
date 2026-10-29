#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_wmiexec, "wmi-exec", "wmi_cimv2_session",
                     "win32_process_create", REX_DISPOSITION_DISPATCHED);
#include "wmi.h"

static HRESULT rex_wmiexec_decode(const rex_arguments *arguments,
                                  rex_request *request) {
    if (rex_wcslen(arguments->values[0]) == 0 ||
        rex_wcslen(arguments->values[1]) != 0 ||
        rex_wcslen(arguments->values[2]) != 0) {
        return E_INVALIDARG;
    }
    request->config.command.text = arguments->values[0];
    return S_OK;
}

static HRESULT rex_wmiexec_preflight(const rex_request *request,
                                     rex_state *state) {
    rex_wmi_session *session = NULL;
    HRESULT result = rex_wmi_open(request->target, &session);
    if (SUCCEEDED(result)) {
        state->private_data = session;
    }
    return result;
}

static HRESULT rex_wmiexec_execute(const rex_request *request, rex_state *state,
                                   rex_observation *observation) {
    HRESULT result;
    if (state->private_data == NULL) {
        return E_UNEXPECTED;
    }
    result = rex_wmi_process_create((rex_wmi_session *)state->private_data,
                                    request->config.command.text,
                                    &observation->process_id);
    if (SUCCEEDED(result)) {
        observation->has_process_id = TRUE;
    }
    return result;
}

static void rex_wmiexec_release(rex_state *state) {
    rex_wmi_close((rex_wmi_session *)state->private_data);
    state->private_data = NULL;
}
