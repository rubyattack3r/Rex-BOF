#include <windows.h>
#include <winsvc.h>

#include "beacon.h"
#include "rex.h"
#include "service.h"

DECLSPEC_IMPORT SC_HANDLE WINAPI ADVAPI32$OpenSCManagerW(LPCWSTR, LPCWSTR, DWORD);
DECLSPEC_IMPORT SC_HANDLE WINAPI ADVAPI32$OpenServiceW(SC_HANDLE, LPCWSTR, DWORD);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$QueryServiceConfigW(
    SC_HANDLE, LPQUERY_SERVICE_CONFIGW, DWORD, LPDWORD);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$QueryServiceStatusEx(
    SC_HANDLE, SC_STATUS_TYPE, LPBYTE, DWORD, LPDWORD);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$ChangeServiceConfigW(
    SC_HANDLE, DWORD, DWORD, DWORD, LPCWSTR, LPCWSTR, LPDWORD, LPCWSTR,
    LPCWSTR, LPCWSTR, LPCWSTR);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$StartServiceW(SC_HANDLE, DWORD, LPCWSTR *);
DECLSPEC_IMPORT SC_HANDLE WINAPI ADVAPI32$CreateServiceW(
    SC_HANDLE, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD, DWORD, LPCWSTR,
    LPCWSTR, LPDWORD, LPCWSTR, LPCWSTR, LPCWSTR);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$CloseServiceHandle(SC_HANDLE);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$GetLastError(void);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

struct rex_service_snapshot {
    SC_HANDLE manager;
    SC_HANDLE service;
    QUERY_SERVICE_CONFIGW *original;
};

struct rex_created_service {
    SC_HANDLE manager;
    SC_HANDLE service;
    wchar_t *name;
};

static HRESULT last_error(void) {
    return HRESULT_FROM_WIN32(KERNEL32$GetLastError());
}

HRESULT rex_service_create_preflight(const char *target, const wchar_t *name,
                                     rex_created_service **out) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_created_service *state;
    wchar_t wide_target[REX_MAX_TARGET + 1];
    LPCWSTR machine = wide_target;
    SC_HANDLE collision;
    SIZE_T bytes;
    DWORD error;
    *out = NULL;
    if (name == NULL || name[0] == L'\0' ||
        !toWideChar((char *)target, wide_target, sizeof(wide_target))) {
        return E_INVALIDARG;
    }
    if (rex_streq(target, ".") || rex_streq(target, "localhost") ||
        rex_streq(target, "127.0.0.1")) {
        machine = NULL;
    }
    state = (rex_created_service *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*state));
    if (state == NULL) {
        return E_OUTOFMEMORY;
    }
    state->manager = ADVAPI32$OpenSCManagerW(
        machine, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (state->manager == NULL) {
        error = KERNEL32$GetLastError();
        KERNEL32$HeapFree(heap, 0, state);
        return HRESULT_FROM_WIN32(error);
    }
    collision = ADVAPI32$OpenServiceW(state->manager, name, SERVICE_QUERY_STATUS);
    if (collision != NULL) {
        ADVAPI32$CloseServiceHandle(collision);
        rex_service_created_close(state);
        return HRESULT_FROM_WIN32(ERROR_SERVICE_EXISTS);
    }
    error = KERNEL32$GetLastError();
    if (error != ERROR_SERVICE_DOES_NOT_EXIST) {
        rex_service_created_close(state);
        return HRESULT_FROM_WIN32(error);
    }
    bytes = (rex_wcslen(name) + 1) * sizeof(wchar_t);
    state->name = (wchar_t *)KERNEL32$HeapAlloc(heap, 0, bytes);
    if (state->name == NULL) {
        rex_service_created_close(state);
        return E_OUTOFMEMORY;
    }
    rex_copy(state->name, name, bytes);
    *out = state;
    return S_OK;
}

HRESULT rex_service_create(rex_created_service *state, const wchar_t *binary_path) {
    state->service = ADVAPI32$CreateServiceW(
        state->manager, state->name, state->name, SERVICE_START | DELETE,
        SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        binary_path, NULL, NULL, NULL, NULL, NULL);
    if (state->service == NULL) {
        return last_error();
    }
    return S_OK;
}

HRESULT rex_service_created_start(rex_created_service *state) {
    return ADVAPI32$StartServiceW(state->service, 0, NULL) ? S_OK : last_error();
}

void rex_service_created_close(rex_created_service *state) {
    HANDLE heap;
    if (state == NULL) {
        return;
    }
    if (state->service != NULL) {
        ADVAPI32$CloseServiceHandle(state->service);
    }
    if (state->manager != NULL) {
        ADVAPI32$CloseServiceHandle(state->manager);
    }
    heap = KERNEL32$GetProcessHeap();
    if (state->name != NULL) {
        KERNEL32$HeapFree(heap, 0, state->name);
    }
    KERNEL32$HeapFree(heap, 0, state);
}

HRESULT rex_service_snapshot_open(const char *target, const wchar_t *name, DWORD access,
                                  rex_service_snapshot **out) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_service_snapshot *value;
    wchar_t wide_target[REX_MAX_TARGET + 1];
    LPCWSTR machine = wide_target;
    SERVICE_STATUS_PROCESS status;
    DWORD required = 0;
    DWORD status_size = 0;
    HRESULT result = S_OK;
    *out = NULL;
    value = (rex_service_snapshot *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*value));
    if (value == NULL) {
        return E_OUTOFMEMORY;
    }
    if (!toWideChar((char *)target, wide_target, sizeof(wide_target))) {
        result = E_INVALIDARG;
        goto fail;
    }
    if (rex_streq(target, ".") || rex_streq(target, "localhost") ||
        rex_streq(target, "127.0.0.1")) {
        machine = NULL;
    }
    value->manager = ADVAPI32$OpenSCManagerW(machine, NULL, SC_MANAGER_CONNECT);
    if (value->manager == NULL) {
        result = last_error();
        goto fail;
    }
    value->service = ADVAPI32$OpenServiceW(value->manager, name,
        SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | access);
    if (value->service == NULL) {
        result = last_error();
        goto fail;
    }
    ADVAPI32$QueryServiceConfigW(value->service, NULL, 0, &required);
    if (required == 0) {
        result = last_error();
        goto fail;
    }
    value->original = (QUERY_SERVICE_CONFIGW *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, required);
    if (value->original == NULL) {
        result = E_OUTOFMEMORY;
        goto fail;
    }
    if (!ADVAPI32$QueryServiceConfigW(
            value->service, value->original, required, &required) ||
        !ADVAPI32$QueryServiceStatusEx(
            value->service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status,
            sizeof(status), &status_size)) {
        result = last_error();
        goto fail;
    }
    if (status.dwCurrentState != SERVICE_STOPPED) {
        result = HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING);
        goto fail;
    }
    *out = value;
    return S_OK;
fail:
    rex_service_snapshot_close(value);
    return result;
}

HRESULT rex_service_change_native(rex_service_snapshot *value, const wchar_t *path) {
    if (!ADVAPI32$ChangeServiceConfigW(
            value->service, SERVICE_NO_CHANGE, SERVICE_DEMAND_START,
            SERVICE_NO_CHANGE, path, NULL, NULL, NULL, NULL, NULL, NULL)) {
        return last_error();
    }
    return S_OK;
}

HRESULT rex_service_start_native(rex_service_snapshot *value) {
    return ADVAPI32$StartServiceW(value->service, 0, NULL) ? S_OK : last_error();
}

const wchar_t *rex_service_original_binary_path(
        const rex_service_snapshot *value) {
    return value == NULL || value->original == NULL
        ? NULL : value->original->lpBinaryPathName;
}

DWORD rex_service_original_start_type(const rex_service_snapshot *value) {
    return value == NULL || value->original == NULL
        ? 0 : value->original->dwStartType;
}

void rex_service_snapshot_close(rex_service_snapshot *value) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    if (value == NULL) {
        return;
    }
    if (value->service != NULL) {
        ADVAPI32$CloseServiceHandle(value->service);
    }
    if (value->manager != NULL) {
        ADVAPI32$CloseServiceHandle(value->manager);
    }
    if (value->original != NULL) {
        KERNEL32$HeapFree(heap, 0, value->original);
    }
    KERNEL32$HeapFree(heap, 0, value);
}
