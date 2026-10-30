#include <windows.h>

#include "rex.h"

#if defined(REX_IDENTITY_TEST)
BOOL WINAPI rex_test_LogonUserW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD,
                                PHANDLE);
BOOL WINAPI rex_test_ImpersonateLoggedOnUser(HANDLE);
BOOL WINAPI rex_test_RevertToSelf(void);
HANDLE WINAPI rex_test_CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T,
                                    LPTHREAD_START_ROUTINE, LPVOID, DWORD,
                                    LPDWORD);
DWORD WINAPI rex_test_WaitForSingleObject(HANDLE, DWORD);
BOOL WINAPI rex_test_CloseHandle(HANDLE);
DWORD WINAPI rex_test_GetLastError(void);
#define REX_LOGON_USER rex_test_LogonUserW
#define REX_IMPERSONATE rex_test_ImpersonateLoggedOnUser
#define REX_REVERT rex_test_RevertToSelf
#define REX_CREATE_THREAD rex_test_CreateThread
#define REX_WAIT rex_test_WaitForSingleObject
#define REX_CLOSE_HANDLE rex_test_CloseHandle
#define REX_GET_LAST_ERROR rex_test_GetLastError
#else
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$LogonUserW(LPCWSTR, LPCWSTR, LPCWSTR,
                                                DWORD, DWORD, PHANDLE);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$ImpersonateLoggedOnUser(HANDLE);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$RevertToSelf(void);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$CreateThread(LPSECURITY_ATTRIBUTES,
                                                    SIZE_T,
                                                    LPTHREAD_START_ROUTINE,
                                                    LPVOID, DWORD, LPDWORD);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$WaitForSingleObject(HANDLE, DWORD);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$CloseHandle(HANDLE);
DECLSPEC_IMPORT DWORD WINAPI KERNEL32$GetLastError(void);
#define REX_LOGON_USER ADVAPI32$LogonUserW
#define REX_IMPERSONATE ADVAPI32$ImpersonateLoggedOnUser
#define REX_REVERT ADVAPI32$RevertToSelf
#define REX_CREATE_THREAD KERNEL32$CreateThread
#define REX_WAIT KERNEL32$WaitForSingleObject
#define REX_CLOSE_HANDLE KERNEL32$CloseHandle
#define REX_GET_LAST_ERROR KERNEL32$GetLastError
#endif

typedef struct {
    const rex_technique *technique;
    rex_arguments *arguments;
    rex_result *result;
    HANDLE token;
} rex_identity_work;

static HRESULT rex_identity_error(void) {
    return HRESULT_FROM_WIN32(REX_GET_LAST_ERROR());
}

static void rex_identity_fail(rex_result *result, rex_phase phase,
                              HRESULT error) {
    result->status = REX_STATUS_FAILED;
    result->phase = phase;
    result->error = error;
}

static BOOL rex_identity_copy(wchar_t *destination, SIZE_T capacity,
                              const wchar_t *source, SIZE_T length) {
    if (destination == NULL || source == NULL || length + 1 > capacity)
        return FALSE;
    rex_copy(destination, source, length * sizeof(wchar_t));
    destination[length] = L'\0';
    return TRUE;
}

static BOOL rex_identity_principal(const wchar_t *principal,
                                   wchar_t domain[256], wchar_t username[256],
                                   const wchar_t **logon_domain,
                                   const wchar_t **logon_username) {
    SIZE_T at = (SIZE_T)-1;
    SIZE_T slash = (SIZE_T)-1;
    SIZE_T index;
    SIZE_T length = rex_wcslen(principal);

    if (length == 0 || length > REX_MAX_PRINCIPAL)
        return FALSE;
    for (index = 0; index < length; index++) {
        wchar_t value = principal[index];
        if (value == L'\\') {
            if (slash != (SIZE_T)-1)
                return FALSE;
            slash = index;
        } else if (value == L'@') {
            if (at != (SIZE_T)-1)
                return FALSE;
            at = index;
        } else if (value < L' ' || value == L'\x7f') {
            return FALSE;
        }
    }
    if (slash != (SIZE_T)-1 && at != (SIZE_T)-1)
        return FALSE;
    if (slash != (SIZE_T)-1) {
        if (slash == 0 || slash + 1 >= length ||
            !rex_identity_copy(domain, 256, principal, slash) ||
            !rex_identity_copy(username, 256, principal + slash + 1,
                               length - slash - 1)) {
            return FALSE;
        }
        *logon_domain = domain;
        *logon_username = username;
        return TRUE;
    }
    if (at == 0 || at == (SIZE_T)-1 || at + 1 >= length || length >= 256)
        return FALSE;
    *logon_domain = NULL;
    *logon_username = principal;
    return TRUE;
}

static DWORD WINAPI rex_identity_worker(LPVOID parameter) {
    rex_identity_work *work = (rex_identity_work *)parameter;
    HRESULT status;

    if (!REX_IMPERSONATE(work->token)) {
        status = rex_identity_error();
        rex_identity_fail(work->result, REX_PHASE_IDENTITY, status);
        return 0;
    }

    rex_lifecycle_run(work->technique, work->arguments, work->result);
    if (!REX_REVERT()) {
        status = rex_identity_error();
        rex_identity_fail(work->result, REX_PHASE_IDENTITY_RESTORE, status);
    }
    return 0;
}

void rex_invoke(const rex_technique *technique, rex_arguments *arguments,
                rex_result *result) {
    rex_identity_work work;
    wchar_t domain[256];
    wchar_t username[256];
    const wchar_t *logon_domain = NULL;
    const wchar_t *logon_username = NULL;
    HANDLE thread;
    HANDLE token = NULL;
    DWORD wait_status;
    HRESULT status;
    SIZE_T principal_length = rex_wcslen(arguments->identity.principal);
    SIZE_T password_length = rex_wcslen(arguments->identity.password);

    result->technique = technique->selector;
    result->target = arguments->target;
    if (principal_length == 0 && password_length == 0) {
        rex_lifecycle_run(technique, arguments, result);
        return;
    }
    if (principal_length == 0 || password_length == 0 ||
        principal_length > REX_MAX_PRINCIPAL ||
        password_length > REX_MAX_PASSWORD) {
        if (arguments->identity.password != NULL)
            rex_zero(arguments->identity.password,
                     (password_length + 1) * sizeof(wchar_t));
        rex_identity_fail(result, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }

    rex_zero(domain, sizeof(domain));
    rex_zero(username, sizeof(username));
    if (!rex_identity_principal(arguments->identity.principal, domain, username,
                                &logon_domain, &logon_username)) {
        rex_zero(arguments->identity.password,
                 (password_length + 1) * sizeof(wchar_t));
        rex_zero(username, sizeof(username));
        rex_zero(domain, sizeof(domain));
        rex_identity_fail(result, REX_PHASE_INPUT, E_INVALIDARG);
        return;
    }
    if (!REX_LOGON_USER(
            logon_username, logon_domain, arguments->identity.password,
            LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &token) ||
        token == NULL) {
        status = rex_identity_error();
        rex_zero(arguments->identity.password,
                 (password_length + 1) * sizeof(wchar_t));
        rex_zero(username, sizeof(username));
        rex_zero(domain, sizeof(domain));
        rex_identity_fail(result, REX_PHASE_IDENTITY, status);
        return;
    }
    rex_zero(arguments->identity.password,
             (password_length + 1) * sizeof(wchar_t));
    rex_zero(username, sizeof(username));
    rex_zero(domain, sizeof(domain));

    work.technique = technique;
    work.arguments = arguments;
    work.result = result;
    work.token = token;
    thread = REX_CREATE_THREAD(NULL, 0, rex_identity_worker, &work, 0, NULL);
    if (thread == NULL) {
        status = rex_identity_error();
        REX_CLOSE_HANDLE(token);
        rex_identity_fail(result, REX_PHASE_IDENTITY, status);
        return;
    }
    wait_status = REX_WAIT(thread, INFINITE);
    if (wait_status != WAIT_OBJECT_0) {
        status = rex_identity_error();
        (void)REX_WAIT(thread, INFINITE);
        rex_identity_fail(result, REX_PHASE_IDENTITY_RESTORE, status);
    }
    REX_CLOSE_HANDLE(thread);
    REX_CLOSE_HANDLE(token);
}

#undef REX_LOGON_USER
#undef REX_IMPERSONATE
#undef REX_REVERT
#undef REX_CREATE_THREAD
#undef REX_WAIT
#undef REX_CLOSE_HANDLE
#undef REX_GET_LAST_ERROR
