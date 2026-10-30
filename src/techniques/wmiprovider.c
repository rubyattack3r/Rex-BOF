#include <windows.h>

#include "provider.h"
#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_wmi_provider, "wmi-provider",
                     "wmi_provider_and_registry_available",
                     "msft_providers_load", REX_DISPOSITION_COMPLETED);

static HRESULT rex_wmi_provider_decode(const rex_arguments *arguments,
                                       rex_request *request) {
    return rex_provider_decode(arguments, request,
                               REX_PROVIDER_ROLE_INITIALIZATION);
}

static HRESULT rex_wmi_provider_preflight(const rex_request *request,
                                          rex_state *state) {
    return rex_provider_preflight(request, state);
}

static HRESULT rex_wmi_provider_execute(const rex_request *request,
                                        rex_state *state,
                                        rex_observation *observation) {
    return rex_provider_execute(request, state, observation);
}

static void rex_wmi_provider_release(rex_state *state) {
    rex_provider_release(state);
}
