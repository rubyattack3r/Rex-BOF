#define COBJMACROS

#include <oleauto.h>
#include <wbemidl.h>
#include <windows.h>

#include "beacon.h"
#include "rex.h"
#include "wmi.h"

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID, DWORD);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance(REFCLSID, LPUNKNOWN,
                                                      DWORD, REFIID, LPVOID *);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown *, DWORD, DWORD,
                                                       OLECHAR *, DWORD, DWORD,
                                                       RPC_AUTH_IDENTITY_HANDLE,
                                                       DWORD);
DECLSPEC_IMPORT BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
DECLSPEC_IMPORT void WINAPI OLEAUT32$SysFreeString(BSTR);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantInit(VARIANTARG *);
DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$VariantClear(VARIANTARG *);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

static const CLSID rex_clsid_wbem_locator = {
    0x4590f811,
    0x1d3a,
    0x11d0,
    {0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};

static const IID rex_iid_wbem_locator = {
    0xdc12a687,
    0x737f,
    0x11cf,
    {0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};

struct rex_wmi_session {
    IWbemServices *services;
    IWbemClassObject *process_class;
    IWbemClassObject *create_input_definition;
    BOOL uninitialize_com;
};

static BOOL rex_append_wide(wchar_t *destination, int capacity, int *offset,
                            const wchar_t *source) {
    int index = 0;
    while (source[index] != L'\0') {
        if (*offset + 1 >= capacity) {
            return FALSE;
        }
        destination[*offset] = source[index];
        (*offset)++;
        index++;
    }
    destination[*offset] = L'\0';
    return TRUE;
}

static BOOL rex_namespace_path(const char *target,
                               const wchar_t *namespace_name,
                               wchar_t output[512]) {
    wchar_t wide_target[REX_MAX_TARGET + 1];
    int offset = 0;
    if (!toWideChar((char *)target, wide_target, sizeof(wide_target))) {
        return FALSE;
    }
    output[0] = L'\0';
    return rex_append_wide(output, 512, &offset, L"\\\\") &&
           rex_append_wide(output, 512, &offset, wide_target) &&
           rex_append_wide(output, 512, &offset, L"\\") &&
           rex_append_wide(output, 512, &offset, namespace_name);
}

static HRESULT rex_variant_dword(VARIANT *value, DWORD *output) {
    if (V_VT(value) == VT_I4 || V_VT(value) == VT_INT) {
        *output = (DWORD)V_I4(value);
        return S_OK;
    }
    if (V_VT(value) == VT_UI4 || V_VT(value) == VT_UINT) {
        *output = V_UI4(value);
        return S_OK;
    }
    if (V_VT(value) == VT_I2) {
        *output = (DWORD)V_I2(value);
        return S_OK;
    }
    return WBEM_E_TYPE_MISMATCH;
}

static HRESULT rex_wmi_method_result(IWbemClassObject *output) {
    VARIANT return_value;
    DWORD method_result = 0;
    HRESULT result;
    OLEAUT32$VariantInit(&return_value);
    result = IWbemClassObject_Get(output, L"ReturnValue", 0, &return_value,
                                  NULL, NULL);
    if (SUCCEEDED(result)) {
        result = rex_variant_dword(&return_value, &method_result);
    }
    OLEAUT32$VariantClear(&return_value);
    if (SUCCEEDED(result) && method_result != 0) {
        result = HRESULT_FROM_WIN32(method_result);
    }
    return result;
}

static HRESULT rex_wmi_service_path(const wchar_t *name, BSTR *path) {
    wchar_t value[512];
    int offset = 0;
    SIZE_T index;
    for (index = 0; name[index] != L'\0'; index++) {
        if (name[index] == L'\'' || name[index] == L'\\') {
            return E_INVALIDARG;
        }
    }
    value[0] = L'\0';
    if (!rex_append_wide(value, 512, &offset, L"Win32_Service.Name='") ||
        !rex_append_wide(value, 512, &offset, name) ||
        !rex_append_wide(value, 512, &offset, L"'")) {
        return E_INVALIDARG;
    }
    *path = OLEAUT32$SysAllocString(value);
    return *path == NULL ? E_OUTOFMEMORY : S_OK;
}

HRESULT rex_wmi_open_namespace(const char *target,
                               const wchar_t *namespace_name,
                               rex_wmi_session **session_out) {
    HRESULT result;
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_wmi_session *session = NULL;
    IWbemLocator *locator = NULL;
    BSTR resource = NULL;
    wchar_t namespace_value[512];

    *session_out = NULL;
    session = (rex_wmi_session *)KERNEL32$HeapAlloc(heap, HEAP_ZERO_MEMORY,
                                                    sizeof(rex_wmi_session));
    if (session == NULL) {
        return E_OUTOFMEMORY;
    }

    result = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK || result == S_FALSE) {
        session->uninitialize_com = TRUE;
    } else if (result != RPC_E_CHANGED_MODE) {
        goto cleanup;
    }

    result = OLE32$CoCreateInstance(&rex_clsid_wbem_locator, NULL,
                                    CLSCTX_INPROC_SERVER, &rex_iid_wbem_locator,
                                    (LPVOID *)&locator);
    if (FAILED(result)) {
        goto cleanup;
    }
    if (!rex_namespace_path(target, namespace_name, namespace_value)) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    resource = OLEAUT32$SysAllocString(namespace_value);
    if (resource == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = IWbemLocator_ConnectServer(locator, resource, NULL, NULL, NULL, 0,
                                        NULL, NULL, &session->services);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = OLE32$CoSetProxyBlanket(
        (IUnknown *)session->services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
        EOAC_DYNAMIC_CLOAKING);
    if (FAILED(result)) {
        goto cleanup;
    }

    result = S_OK;

cleanup:
    if (resource != NULL) {
        OLEAUT32$SysFreeString(resource);
    }
    if (locator != NULL) {
        IWbemLocator_Release(locator);
    }
    if (FAILED(result)) {
        rex_wmi_close(session);
        return result;
    }
    *session_out = session;
    return S_OK;
}

HRESULT rex_wmi_open(const char *target, rex_wmi_session **session_out) {
    return rex_wmi_open_namespace(target, L"ROOT\\CIMV2", session_out);
}

HRESULT rex_wmi_process_create(rex_wmi_session *session,
                               const wchar_t *command_text, DWORD *process_id) {
    HRESULT result;
    BSTR command = NULL;
    BSTR process_path = NULL;
    IWbemClassObject *input = NULL;
    IWbemClassObject *output = NULL;
    VARIANT command_value;
    VARIANT return_value;
    VARIANT pid_value;
    DWORD method_result = 0;

    OLEAUT32$VariantInit(&command_value);
    OLEAUT32$VariantInit(&return_value);
    OLEAUT32$VariantInit(&pid_value);

    if (session->create_input_definition == NULL) {
        process_path = OLEAUT32$SysAllocString(L"Win32_Process");
        if (process_path == NULL) {
            result = E_OUTOFMEMORY;
            goto cleanup;
        }
        result = IWbemServices_GetObject(session->services, process_path,
                                         WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL,
                                         &session->process_class, NULL);
        if (FAILED(result)) {
            goto cleanup;
        }
        result =
            IWbemClassObject_GetMethod(session->process_class, L"Create", 0,
                                       &session->create_input_definition, NULL);
        if (FAILED(result)) {
            goto cleanup;
        }
    }

    command = OLEAUT32$SysAllocString(command_text);
    if (command == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = IWbemClassObject_SpawnInstance(session->create_input_definition, 0,
                                            &input);
    if (FAILED(result)) {
        goto cleanup;
    }
    V_VT(&command_value) = VT_BSTR;
    V_BSTR(&command_value) = command;
    result = IWbemClassObject_Put(input, L"CommandLine", 0, &command_value, 0);
    if (FAILED(result)) {
        goto cleanup;
    }

    if (process_path == NULL) {
        process_path = OLEAUT32$SysAllocString(L"Win32_Process");
        if (process_path == NULL) {
            result = E_OUTOFMEMORY;
            goto cleanup;
        }
    }
    result = IWbemServices_ExecMethod(session->services, process_path,
                                      L"Create", 0, NULL, input, &output, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWbemClassObject_Get(output, L"ReturnValue", 0, &return_value,
                                  NULL, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = rex_variant_dword(&return_value, &method_result);
    if (FAILED(result)) {
        goto cleanup;
    }
    if (method_result != 0) {
        result = HRESULT_FROM_WIN32(method_result);
        goto cleanup;
    }
    result =
        IWbemClassObject_Get(output, L"ProcessId", 0, &pid_value, NULL, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = rex_variant_dword(&pid_value, process_id);

cleanup:
    if (output != NULL) {
        IWbemClassObject_Release(output);
    }
    if (input != NULL) {
        IWbemClassObject_Release(input);
    }
    OLEAUT32$VariantClear(&pid_value);
    OLEAUT32$VariantClear(&return_value);
    V_VT(&command_value) = VT_EMPTY;
    if (command != NULL) {
        OLEAUT32$SysFreeString(command);
    }
    if (process_path != NULL) {
        OLEAUT32$SysFreeString(process_path);
    }
    return result;
}

HRESULT rex_wmi_service_change(rex_wmi_session *session, const wchar_t *name,
                               const wchar_t *binary_path) {
    BSTR class_path = NULL;
    BSTR object_path = NULL;
    BSTR path_value = NULL;
    IWbemClassObject *service_class = NULL;
    IWbemClassObject *input_definition = NULL;
    IWbemClassObject *input = NULL;
    IWbemClassObject *output = NULL;
    VARIANT path;
    HRESULT result;

    OLEAUT32$VariantInit(&path);
    class_path = OLEAUT32$SysAllocString(L"Win32_Service");
    if (class_path == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = rex_wmi_service_path(name, &object_path);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWbemServices_GetObject(session->services, class_path, 0, NULL,
                                     &service_class, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWbemClassObject_GetMethod(service_class, L"Change", 0,
                                        &input_definition, NULL);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWbemClassObject_SpawnInstance(input_definition, 0, &input);
    if (FAILED(result)) {
        goto cleanup;
    }
    path_value = OLEAUT32$SysAllocString(binary_path);
    if (path_value == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    V_VT(&path) = VT_BSTR;
    V_BSTR(&path) = path_value;
    result = IWbemClassObject_Put(input, L"PathName", 0, &path, 0);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWbemServices_ExecMethod(session->services, object_path, L"Change",
                                      0, NULL, input, &output, NULL);
    if (SUCCEEDED(result)) {
        result = rex_wmi_method_result(output);
    }

cleanup:
    if (output != NULL) {
        IWbemClassObject_Release(output);
    }
    if (input != NULL) {
        IWbemClassObject_Release(input);
    }
    if (input_definition != NULL) {
        IWbemClassObject_Release(input_definition);
    }
    if (service_class != NULL) {
        IWbemClassObject_Release(service_class);
    }
    V_VT(&path) = VT_EMPTY;
    if (path_value != NULL) {
        OLEAUT32$SysFreeString(path_value);
    }
    if (object_path != NULL) {
        OLEAUT32$SysFreeString(object_path);
    }
    if (class_path != NULL) {
        OLEAUT32$SysFreeString(class_path);
    }
    return result;
}

HRESULT rex_wmi_service_start(rex_wmi_session *session, const wchar_t *name) {
    BSTR object_path = NULL;
    IWbemClassObject *output = NULL;
    HRESULT result = rex_wmi_service_path(name, &object_path);
    if (SUCCEEDED(result)) {
        result = IWbemServices_ExecMethod(session->services, object_path,
                                          L"StartService", 0, NULL, NULL,
                                          &output, NULL);
    }
    if (SUCCEEDED(result)) {
        result = rex_wmi_method_result(output);
    }
    if (output != NULL) {
        IWbemClassObject_Release(output);
    }
    if (object_path != NULL) {
        OLEAUT32$SysFreeString(object_path);
    }
    return result;
}

static HRESULT rex_provider_path(const wchar_t *name, BSTR *path) {
    wchar_t value[512];
    int offset = 0;
    value[0] = L'\0';
    if (!rex_append_wide(value, 512, &offset, L"__Win32Provider.Name='") ||
        !rex_append_wide(value, 512, &offset, name) ||
        !rex_append_wide(value, 512, &offset, L"'"))
        return E_INVALIDARG;
    *path = OLEAUT32$SysAllocString(value);
    return *path == NULL ? E_OUTOFMEMORY : S_OK;
}

HRESULT rex_wmi_provider_exists(rex_wmi_session *session, const wchar_t *name,
                                BOOL *exists) {
    BSTR path = NULL;
    IWbemClassObject *object = NULL;
    HRESULT result = rex_provider_path(name, &path);
    *exists = FALSE;
    if (SUCCEEDED(result))
        result = IWbemServices_GetObject(session->services, path, 0, NULL,
                                         &object, NULL);
    if (result == (HRESULT)WBEM_E_NOT_FOUND)
        result = S_OK;
    else if (SUCCEEDED(result))
        *exists = TRUE;
    if (object != NULL)
        IWbemClassObject_Release(object);
    if (path != NULL)
        OLEAUT32$SysFreeString(path);
    return result;
}

HRESULT rex_wmi_provider_register(rex_wmi_session *session, const wchar_t *name,
                                  const wchar_t *class_id) {
    BSTR class_path = NULL, name_value = NULL, class_value = NULL,
         host_value = NULL;
    IWbemClassObject *provider_class = NULL, *instance = NULL;
    VARIANT value;
    wchar_t hosting[512];
    int offset = 0;
    HRESULT result;
    OLEAUT32$VariantInit(&value);
    hosting[0] = L'\0';
    if (!rex_append_wide(hosting, 512, &offset, L"LocalSystemHost:") ||
        !rex_append_wide(hosting, 512, &offset, name))
        return E_INVALIDARG;
    class_path = OLEAUT32$SysAllocString(L"__Win32Provider");
    name_value = OLEAUT32$SysAllocString(name);
    class_value = OLEAUT32$SysAllocString(class_id);
    host_value = OLEAUT32$SysAllocString(hosting);
    if (class_path == NULL || name_value == NULL || class_value == NULL ||
        host_value == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = IWbemServices_GetObject(session->services, class_path, 0, NULL,
                                     &provider_class, NULL);
    if (FAILED(result))
        goto cleanup;
    result = IWbemClassObject_SpawnInstance(provider_class, 0, &instance);
    if (FAILED(result))
        goto cleanup;
#define REX_PUT_PROVIDER(property, bstr)                                       \
    V_VT(&value) = VT_BSTR;                                                    \
    V_BSTR(&value) = (bstr);                                                   \
    result = IWbemClassObject_Put(instance, (property), 0, &value, 0);         \
    V_VT(&value) = VT_EMPTY;                                                   \
    if (FAILED(result))                                                        \
    goto cleanup
    REX_PUT_PROVIDER(L"Name", name_value);
    REX_PUT_PROVIDER(L"Clsid", class_value);
    REX_PUT_PROVIDER(L"HostingModel", host_value);
#undef REX_PUT_PROVIDER
    result = IWbemServices_PutInstance(session->services, instance,
                                       WBEM_FLAG_CREATE_ONLY, NULL, NULL);
cleanup:
    if (instance != NULL)
        IWbemClassObject_Release(instance);
    if (provider_class != NULL)
        IWbemClassObject_Release(provider_class);
    if (host_value != NULL)
        OLEAUT32$SysFreeString(host_value);
    if (class_value != NULL)
        OLEAUT32$SysFreeString(class_value);
    if (name_value != NULL)
        OLEAUT32$SysFreeString(name_value);
    if (class_path != NULL)
        OLEAUT32$SysFreeString(class_path);
    return result;
}

HRESULT rex_wmi_provider_delete(rex_wmi_session *session, const wchar_t *name) {
    BSTR path = NULL;
    HRESULT result = rex_provider_path(name, &path);
    if (SUCCEEDED(result))
        result = IWbemServices_DeleteInstance(session->services, path, 0, NULL,
                                              NULL);
    if (path != NULL)
        OLEAUT32$SysFreeString(path);
    return result == (HRESULT)WBEM_E_NOT_FOUND ? S_OK : result;
}

static HRESULT rex_wmi_provider_control(rex_wmi_session *session,
                                        const wchar_t *method,
                                        const wchar_t *name) {
    BSTR class_path = NULL, method_value = NULL, namespace_value = NULL,
         name_value = NULL;
    IWbemClassObject *provider_class = NULL, *input_definition = NULL;
    IWbemClassObject *input = NULL, *output = NULL;
    VARIANT value;
    HRESULT result;
    OLEAUT32$VariantInit(&value);
    class_path = OLEAUT32$SysAllocString(L"MSFT_Providers");
    method_value = OLEAUT32$SysAllocString(method);
    namespace_value = OLEAUT32$SysAllocString(L"root\\default");
    name_value = OLEAUT32$SysAllocString(name);
    if (class_path == NULL || method_value == NULL || namespace_value == NULL ||
        name_value == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = IWbemServices_GetObject(session->services, class_path, 0, NULL,
                                     &provider_class, NULL);
    if (FAILED(result))
        goto cleanup;
    result = IWbemClassObject_GetMethod(provider_class, method, 0,
                                        &input_definition, NULL);
    if (FAILED(result))
        goto cleanup;
    result = IWbemClassObject_SpawnInstance(input_definition, 0, &input);
    if (FAILED(result))
        goto cleanup;
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = namespace_value;
    result = IWbemClassObject_Put(input, L"Namespace", 0, &value, 0);
    V_VT(&value) = VT_EMPTY;
    if (FAILED(result))
        goto cleanup;
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = name_value;
    result = IWbemClassObject_Put(input, L"provider", 0, &value, 0);
    V_VT(&value) = VT_EMPTY;
    if (FAILED(result))
        goto cleanup;
    V_VT(&value) = VT_NULL;
    result = IWbemClassObject_Put(input, L"User", 0, &value, 0);
    if (FAILED(result))
        goto cleanup;
    result = IWbemClassObject_Put(input, L"Locale", 0, &value, 0);
    if (FAILED(result))
        goto cleanup;
    result =
        IWbemClassObject_Put(input, L"TransactionIdentifier", 0, &value, 0);
    V_VT(&value) = VT_EMPTY;
    if (FAILED(result))
        goto cleanup;
    result =
        IWbemServices_ExecMethod(session->services, class_path, method_value, 0,
                                 NULL, input, &output, NULL);
    if (SUCCEEDED(result) && output != NULL)
        result = rex_wmi_method_result(output);
cleanup:
    if (output != NULL)
        IWbemClassObject_Release(output);
    if (input != NULL)
        IWbemClassObject_Release(input);
    if (input_definition != NULL)
        IWbemClassObject_Release(input_definition);
    if (provider_class != NULL)
        IWbemClassObject_Release(provider_class);
    if (name_value != NULL)
        OLEAUT32$SysFreeString(name_value);
    if (namespace_value != NULL)
        OLEAUT32$SysFreeString(namespace_value);
    if (class_path != NULL)
        OLEAUT32$SysFreeString(class_path);
    if (method_value != NULL)
        OLEAUT32$SysFreeString(method_value);
    return result;
}

HRESULT rex_wmi_provider_load(rex_wmi_session *session, const wchar_t *name) {
    return rex_wmi_provider_control(session, L"Load", name);
}

static wchar_t rex_ascii_lower(wchar_t value) {
    if (value >= L'A' && value <= L'Z')
        return value + (L'a' - L'A');
    return value;
}

static BOOL rex_wmi_value_equal(const wchar_t *left, const wchar_t *right) {
    SIZE_T index = 0;
    if (left == NULL || right == NULL)
        return FALSE;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (rex_ascii_lower(left[index]) != rex_ascii_lower(right[index]))
            return FALSE;
        index++;
    }
    return left[index] == right[index];
}

HRESULT rex_wmi_provider_unload(rex_wmi_session *session, const wchar_t *name) {
    BSTR language = NULL, query = NULL, method = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *instance = NULL, *output = NULL;
    VARIANT provider, namespace_value, path;
    ULONG returned = 0;
    HRESULT result;
    language = OLEAUT32$SysAllocString(L"WQL");
    query = OLEAUT32$SysAllocString(L"SELECT * FROM MSFT_Providers");
    method = OLEAUT32$SysAllocString(L"UnLoad");
    if (language == NULL || query == NULL || method == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = IWbemServices_ExecQuery(session->services, language, query,
                                     WBEM_FLAG_FORWARD_ONLY |
                                         WBEM_FLAG_RETURN_IMMEDIATELY,
                                     NULL, &enumerator);
    if (FAILED(result))
        goto cleanup;
    for (;;) {
        OLEAUT32$VariantInit(&provider);
        OLEAUT32$VariantInit(&namespace_value);
        OLEAUT32$VariantInit(&path);
        result = IEnumWbemClassObject_Next(enumerator, WBEM_INFINITE, 1,
                                           &instance, &returned);
        if (FAILED(result) || returned == 0)
            break;
        result = IWbemClassObject_Get(instance, L"provider", 0, &provider, NULL,
                                      NULL);
        if (SUCCEEDED(result))
            result = IWbemClassObject_Get(instance, L"Namespace", 0,
                                          &namespace_value, NULL, NULL);
        if (SUCCEEDED(result) && V_VT(&provider) == VT_BSTR &&
            V_VT(&namespace_value) == VT_BSTR &&
            rex_wmi_value_equal(V_BSTR(&provider), name) &&
            rex_wmi_value_equal(V_BSTR(&namespace_value), L"root\\default")) {
            result =
                IWbemClassObject_Get(instance, L"__PATH", 0, &path, NULL, NULL);
            if (SUCCEEDED(result) && V_VT(&path) == VT_BSTR) {
                result = IWbemServices_ExecMethod(session->services,
                                                  V_BSTR(&path), method, 0,
                                                  NULL, NULL, &output, NULL);
                if (SUCCEEDED(result) && output != NULL)
                    result = rex_wmi_method_result(output);
            } else if (SUCCEEDED(result)) {
                result = WBEM_E_TYPE_MISMATCH;
            }
        } else if (SUCCEEDED(result)) {
            result = S_FALSE;
        }
        if (output != NULL) {
            IWbemClassObject_Release(output);
            output = NULL;
        }
        IWbemClassObject_Release(instance);
        instance = NULL;
        OLEAUT32$VariantClear(&path);
        OLEAUT32$VariantClear(&namespace_value);
        OLEAUT32$VariantClear(&provider);
        if (result != S_FALSE)
            break;
    }
    if (result == S_FALSE || (SUCCEEDED(result) && returned == 0))
        result = S_OK;
cleanup:
    if (output != NULL)
        IWbemClassObject_Release(output);
    if (instance != NULL)
        IWbemClassObject_Release(instance);
    if (enumerator != NULL)
        IEnumWbemClassObject_Release(enumerator);
    if (method != NULL)
        OLEAUT32$SysFreeString(method);
    if (query != NULL)
        OLEAUT32$SysFreeString(query);
    if (language != NULL)
        OLEAUT32$SysFreeString(language);
    return result;
}

static HRESULT rex_wmi_named_path(const wchar_t *class_name,
                                  const wchar_t *name, BSTR *path) {
    wchar_t value[512];
    int offset = 0;
    SIZE_T index;
    if (class_name == NULL || name == NULL || name[0] == L'\0') {
        return E_INVALIDARG;
    }
    for (index = 0; name[index] != L'\0'; index++) {
        if (name[index] == L'\'' || name[index] == L'\\') {
            return E_INVALIDARG;
        }
    }
    value[0] = L'\0';
    if (!rex_append_wide(value, 512, &offset, class_name) ||
        !rex_append_wide(value, 512, &offset, L".Name='") ||
        !rex_append_wide(value, 512, &offset, name) ||
        !rex_append_wide(value, 512, &offset, L"'")) {
        return E_INVALIDARG;
    }
    *path = OLEAUT32$SysAllocString(value);
    return *path == NULL ? E_OUTOFMEMORY : S_OK;
}

static HRESULT rex_wmi_named_exists(rex_wmi_session *session,
                                    const wchar_t *class_name,
                                    const wchar_t *name, BOOL *exists) {
    BSTR path = NULL;
    IWbemClassObject *object = NULL;
    HRESULT result = rex_wmi_named_path(class_name, name, &path);
    *exists = FALSE;
    if (SUCCEEDED(result)) {
        result = IWbemServices_GetObject(session->services, path, 0, NULL,
                                         &object, NULL);
    }
    if (result == (HRESULT)WBEM_E_NOT_FOUND) {
        result = S_OK;
    } else if (SUCCEEDED(result)) {
        *exists = TRUE;
    }
    if (object != NULL)
        IWbemClassObject_Release(object);
    if (path != NULL)
        OLEAUT32$SysFreeString(path);
    return result;
}

static HRESULT rex_wmi_put_string(IWbemClassObject *instance,
                                  const wchar_t *property,
                                  const wchar_t *text) {
    BSTR value_text = OLEAUT32$SysAllocString(text);
    VARIANT value;
    HRESULT result;
    OLEAUT32$VariantInit(&value);
    if (value_text == NULL)
        return E_OUTOFMEMORY;
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = value_text;
    result = IWbemClassObject_Put(instance, property, 0, &value, 0);
    V_VT(&value) = VT_EMPTY;
    OLEAUT32$SysFreeString(value_text);
    return result;
}

static HRESULT rex_wmi_spawn(rex_wmi_session *session,
                             const wchar_t *class_name,
                             IWbemClassObject **instance) {
    BSTR path = OLEAUT32$SysAllocString(class_name);
    IWbemClassObject *definition = NULL;
    HRESULT result;
    *instance = NULL;
    if (path == NULL)
        return E_OUTOFMEMORY;
    result = IWbemServices_GetObject(session->services, path, 0, NULL,
                                     &definition, NULL);
    if (SUCCEEDED(result)) {
        result = IWbemClassObject_SpawnInstance(definition, 0, instance);
    }
    if (definition != NULL)
        IWbemClassObject_Release(definition);
    OLEAUT32$SysFreeString(path);
    return result;
}

HRESULT rex_wmi_subscription_preflight(rex_wmi_session *session,
                                       const wchar_t *name) {
    BOOL exists = FALSE;
    HRESULT result =
        rex_wmi_named_exists(session, L"__EventFilter", name, &exists);
    if (FAILED(result))
        return result;
    if (exists)
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    result = rex_wmi_named_exists(session, L"CommandLineEventConsumer", name,
                                  &exists);
    if (FAILED(result))
        return result;
    return exists ? HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) : S_OK;
}

HRESULT rex_wmi_subscription_create(rex_wmi_session *session,
                                    const wchar_t *name, const wchar_t *query,
                                    const wchar_t *command, DWORD *created) {
    IWbemClassObject *instance = NULL;
    VARIANT synchronous;
    wchar_t filter_path[512], consumer_path[512];
    int filter_offset = 0, consumer_offset = 0;
    HRESULT result;
    *created = 0;

    result = rex_wmi_spawn(session, L"__EventFilter", &instance);
    if (FAILED(result))
        goto cleanup;
    result = rex_wmi_put_string(instance, L"Name", name);
    if (SUCCEEDED(result))
        result =
            rex_wmi_put_string(instance, L"EventNamespace", L"root\\cimv2");
    if (SUCCEEDED(result))
        result = rex_wmi_put_string(instance, L"QueryLanguage", L"WQL");
    if (SUCCEEDED(result))
        result = rex_wmi_put_string(instance, L"Query", query);
    if (SUCCEEDED(result))
        result = IWbemServices_PutInstance(session->services, instance,
                                           WBEM_FLAG_CREATE_ONLY, NULL, NULL);
    IWbemClassObject_Release(instance);
    instance = NULL;
    if (FAILED(result))
        goto cleanup;
    *created |= 1;

    result = rex_wmi_spawn(session, L"CommandLineEventConsumer", &instance);
    if (FAILED(result))
        goto cleanup;
    result = rex_wmi_put_string(instance, L"Name", name);
    if (SUCCEEDED(result))
        result = rex_wmi_put_string(instance, L"CommandLineTemplate", command);
    if (SUCCEEDED(result))
        result = IWbemServices_PutInstance(session->services, instance,
                                           WBEM_FLAG_CREATE_ONLY, NULL, NULL);
    IWbemClassObject_Release(instance);
    instance = NULL;
    if (FAILED(result))
        goto cleanup;
    *created |= 2;

    filter_path[0] = L'\0';
    consumer_path[0] = L'\0';
    if (!rex_append_wide(filter_path, 512, &filter_offset,
                         L"__EventFilter.Name=\"") ||
        !rex_append_wide(filter_path, 512, &filter_offset, name) ||
        !rex_append_wide(filter_path, 512, &filter_offset, L"\"") ||
        !rex_append_wide(consumer_path, 512, &consumer_offset,
                         L"CommandLineEventConsumer.Name=\"") ||
        !rex_append_wide(consumer_path, 512, &consumer_offset, name) ||
        !rex_append_wide(consumer_path, 512, &consumer_offset, L"\"")) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    result = rex_wmi_spawn(session, L"__FilterToConsumerBinding", &instance);
    if (FAILED(result))
        goto cleanup;
    result = rex_wmi_put_string(instance, L"Filter", filter_path);
    if (SUCCEEDED(result))
        result = rex_wmi_put_string(instance, L"Consumer", consumer_path);
    OLEAUT32$VariantInit(&synchronous);
    if (SUCCEEDED(result)) {
        V_VT(&synchronous) = VT_BOOL;
        V_BOOL(&synchronous) = VARIANT_FALSE;
        result = IWbemClassObject_Put(instance, L"DeliverSynchronously", 0,
                                      &synchronous, 0);
    }
    if (SUCCEEDED(result))
        result = IWbemServices_PutInstance(session->services, instance,
                                           WBEM_FLAG_CREATE_ONLY, NULL, NULL);
    if (SUCCEEDED(result))
        *created |= 4;

cleanup:
    if (instance != NULL)
        IWbemClassObject_Release(instance);
    return result;
}

void rex_wmi_close(rex_wmi_session *session) {
    HANDLE heap;
    if (session == NULL) {
        return;
    }
    if (session->create_input_definition != NULL) {
        IWbemClassObject_Release(session->create_input_definition);
    }
    if (session->process_class != NULL) {
        IWbemClassObject_Release(session->process_class);
    }
    if (session->services != NULL) {
        IWbemServices_Release(session->services);
    }
    if (session->uninitialize_com) {
        OLE32$CoUninitialize();
    }
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, session);
}
