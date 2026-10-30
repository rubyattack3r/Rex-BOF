#include <windows.h>

#include "rex.h"
#include "technique.h"
#include "wmi.h"
#include "wmi_text.h"

REX_DEFINE_TECHNIQUE(rex_wmi_event_subscription, "wmi-event-subscription",
                     "wmi_subscription_name_available",
                     "wmi_filter_consumer_binding_created",
                     REX_DISPOSITION_COMPLETED);

static HRESULT rex_wmi_event_subscription_decode(const rex_arguments *arguments,
                                                 rex_request *request) {
    if (!rex_wmi_name_valid(arguments->values[0]) ||
        arguments->values[1][0] == L'\0' || arguments->values[2][0] == L'\0') {
        return E_INVALIDARG;
    }
    request->config.subscription.name = arguments->values[0];
    request->config.subscription.query = arguments->values[1];
    request->config.subscription.command = arguments->values[2];
    return S_OK;
}

static HRESULT rex_wmi_event_subscription_preflight(const rex_request *request,
                                                    rex_state *state) {
    rex_wmi_session *session = NULL;
    HRESULT result = rex_wmi_open_namespace(request->target,
                                            L"ROOT\\SUBSCRIPTION", &session);
    if (SUCCEEDED(result)) {
        result = rex_wmi_subscription_preflight(
            session, request->config.subscription.name);
    }
    if (SUCCEEDED(result)) {
        state->private_data = session;
    } else if (session != NULL) {
        rex_wmi_close(session);
    }
    return result;
}

static HRESULT
rex_wmi_event_subscription_execute(const rex_request *request, rex_state *state,
                                   rex_observation *observation) {
    DWORD created = 0;
    HRESULT result = rex_wmi_subscription_create(
        (rex_wmi_session *)state->private_data,
        request->config.subscription.name, request->config.subscription.query,
        request->config.subscription.command, &created);
    if (created != 0) {
        observation->recovery.kind = REX_RECOVERY_REMOVE_WMI_SUBSCRIPTION;
        observation->recovery.value.subscription.name =
            request->config.subscription.name;
    }
    return result;
}

static void rex_wmi_event_subscription_release(rex_state *state) {
    rex_wmi_close((rex_wmi_session *)state->private_data);
    state->private_data = NULL;
}
