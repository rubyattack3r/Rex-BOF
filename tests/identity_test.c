#include <stdio.h>
#include <windows.h>

#include "rex.h"

#define TEST_TOKEN ((HANDLE)(UINT_PTR)0x1234)
#define TEST_THREAD ((HANDLE)(UINT_PTR)0x5678)

static BOOL logon_succeeds;
static BOOL impersonation_succeeds;
static BOOL revert_succeeds;
static BOOL create_thread_succeeds;
static BOOL impersonating;
static BOOL in_worker;
static unsigned int logon_calls;
static unsigned int thread_calls;
static unsigned int lifecycle_calls;
static unsigned int failures;
static const wchar_t *expected_username;
static const wchar_t *expected_domain;

static BOOL wide_equal(const wchar_t *left, const wchar_t *right) {
    SIZE_T index = 0;
    if (left == NULL || right == NULL)
        return left == right;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (left[index] != right[index])
            return FALSE;
        index++;
    }
    return left[index] == right[index];
}

BOOL WINAPI rex_test_LogonUserW(LPCWSTR username, LPCWSTR domain,
                                LPCWSTR password, DWORD logon_type,
                                DWORD provider, PHANDLE token) {
    logon_calls++;
    if (in_worker)
        failures++;
    if (!wide_equal(username, expected_username) ||
        !wide_equal(domain, expected_domain) ||
        !wide_equal(password, L"secret") ||
        logon_type != LOGON32_LOGON_NEW_CREDENTIALS ||
        provider != LOGON32_PROVIDER_WINNT50) {
        failures++;
    }
    if (!logon_succeeds) {
        *token = NULL;
        return FALSE;
    }
    *token = TEST_TOKEN;
    return TRUE;
}

BOOL WINAPI rex_test_ImpersonateLoggedOnUser(HANDLE token) {
    if (token != TEST_TOKEN)
        failures++;
    if (!impersonation_succeeds)
        return FALSE;
    impersonating = TRUE;
    return TRUE;
}

BOOL WINAPI rex_test_RevertToSelf(void) {
    if (!impersonating)
        failures++;
    if (!revert_succeeds)
        return FALSE;
    impersonating = FALSE;
    return TRUE;
}

HANDLE WINAPI rex_test_CreateThread(LPSECURITY_ATTRIBUTES attributes,
                                    SIZE_T stack_size,
                                    LPTHREAD_START_ROUTINE start,
                                    LPVOID parameter, DWORD flags,
                                    LPDWORD identifier) {
    thread_calls++;
    if (attributes != NULL || stack_size != 0 || start == NULL || flags != 0 ||
        identifier != NULL) {
        failures++;
    }
    if (!create_thread_succeeds)
        return NULL;
    in_worker = TRUE;
    (void)start(parameter);
    in_worker = FALSE;
    return TEST_THREAD;
}

DWORD WINAPI rex_test_WaitForSingleObject(HANDLE thread, DWORD milliseconds) {
    if (thread != TEST_THREAD || milliseconds != INFINITE)
        failures++;
    return WAIT_OBJECT_0;
}

BOOL WINAPI rex_test_CloseHandle(HANDLE handle) {
    if (handle != TEST_THREAD && handle != TEST_TOKEN)
        failures++;
    return TRUE;
}

DWORD WINAPI rex_test_GetLastError(void) {
    return ERROR_LOGON_FAILURE;
}

void rex_lifecycle_run(const rex_technique *technique,
                       const rex_arguments *arguments, rex_result *result) {
    lifecycle_calls++;
    if (technique == NULL || arguments == NULL)
        failures++;
    if (logon_calls > 0 && !impersonating)
        failures++;
    result->status = REX_STATUS_SUCCEEDED;
    result->disposition = REX_DISPOSITION_VALIDATED;
}

static void reset_state(void) {
    logon_succeeds = TRUE;
    impersonation_succeeds = TRUE;
    revert_succeeds = TRUE;
    create_thread_succeeds = TRUE;
    impersonating = FALSE;
    in_worker = FALSE;
    logon_calls = 0;
    thread_calls = 0;
    lifecycle_calls = 0;
    expected_username = NULL;
    expected_domain = NULL;
}

static void run_case(const wchar_t *principal, wchar_t *password,
                     rex_result *result) {
    rex_technique technique = {0};
    rex_arguments arguments = {0};
    technique.selector = "wmi-exec";
    arguments.target = "server01";
    arguments.identity.principal = principal;
    arguments.identity.password = password;
    rex_invoke(&technique, &arguments, result);
}

static void expect_cleared(const wchar_t *password, SIZE_T capacity) {
    SIZE_T index;
    for (index = 0; index < capacity; index++) {
        if (password[index] != L'\0')
            failures++;
    }
}

int main(void) {
    rex_result result = {0};
    wchar_t empty[] = L"";
    wchar_t password1[7] = L"secret";
    wchar_t password2[7] = L"secret";
    wchar_t password3[7] = L"secret";
    wchar_t password4[7] = L"secret";
    wchar_t password5[7] = L"secret";
    wchar_t password6[7] = L"secret";

    reset_state();
    run_case(L"", empty, &result);
    if (lifecycle_calls != 1 || thread_calls != 0 || logon_calls != 0 ||
        result.status != REX_STATUS_SUCCEEDED) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    run_case(L"DOMAIN\\operator", password1, &result);
    expect_cleared(password1, 7);
    if (lifecycle_calls != 1 || thread_calls != 1 || logon_calls != 1 ||
        impersonating || result.status != REX_STATUS_SUCCEEDED) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    expected_username = L"operator@example.com";
    expected_domain = NULL;
    run_case(L"operator@example.com", password2, &result);
    expect_cleared(password2, 7);
    if (lifecycle_calls != 1 || logon_calls != 1 || impersonating ||
        result.status != REX_STATUS_SUCCEEDED) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    run_case(L"operator", password3, &result);
    expect_cleared(password3, 7);
    if (lifecycle_calls != 0 || logon_calls != 0 ||
        result.phase != REX_PHASE_INPUT) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    logon_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password4, &result);
    expect_cleared(password4, 7);
    if (lifecycle_calls != 0 || result.phase != REX_PHASE_IDENTITY) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    impersonation_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password5, &result);
    expect_cleared(password5, 7);
    if (lifecycle_calls != 0 || result.phase != REX_PHASE_IDENTITY) {
        failures++;
    }

    reset_state();
    rex_zero(&result, sizeof(result));
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    revert_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password6, &result);
    expect_cleared(password6, 7);
    if (lifecycle_calls != 1 || result.phase != REX_PHASE_IDENTITY_RESTORE) {
        failures++;
    }

    if (failures != 0) {
        fprintf(stderr, "identity tests failed: %u\n", failures);
        return 1;
    }
    puts("identity tests passed");
    return 0;
}
