#include <windows.h>

#include "rex.h"
#include "utest.h"

#define TEST_TOKEN ((HANDLE)(UINT_PTR)0x1234)
#define TEST_THREAD ((HANDLE)(UINT_PTR)0x5678)

static BOOL logon_succeeds;
static BOOL impersonation_succeeds;
static BOOL revert_succeeds;
static BOOL create_thread_succeeds;
static BOOL impersonating;
static BOOL in_worker;
static BOOL calls_valid;
static unsigned int logon_calls;
static unsigned int thread_calls;
static unsigned int lifecycle_calls;
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
    if (in_worker || !wide_equal(username, expected_username) ||
        !wide_equal(domain, expected_domain) ||
        !wide_equal(password, L"secret") ||
        logon_type != LOGON32_LOGON_NEW_CREDENTIALS ||
        provider != LOGON32_PROVIDER_WINNT50) {
        calls_valid = FALSE;
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
        calls_valid = FALSE;
    if (!impersonation_succeeds)
        return FALSE;
    impersonating = TRUE;
    return TRUE;
}

BOOL WINAPI rex_test_RevertToSelf(void) {
    if (!impersonating)
        calls_valid = FALSE;
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
        calls_valid = FALSE;
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
        calls_valid = FALSE;
    return WAIT_OBJECT_0;
}

BOOL WINAPI rex_test_CloseHandle(HANDLE handle) {
    if (handle != TEST_THREAD && handle != TEST_TOKEN)
        calls_valid = FALSE;
    return TRUE;
}

DWORD WINAPI rex_test_GetLastError(void) {
    return ERROR_LOGON_FAILURE;
}

void rex_lifecycle_run(const rex_technique *technique,
                       const rex_arguments *arguments, rex_result *result) {
    lifecycle_calls++;
    if (technique == NULL || arguments == NULL ||
        (logon_calls > 0 && !impersonating)) {
        calls_valid = FALSE;
    }
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
    calls_valid = TRUE;
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

static BOOL password_cleared(const wchar_t *password, SIZE_T capacity) {
    SIZE_T index;
    for (index = 0; index < capacity; index++) {
        if (password[index] != L'\0')
            return FALSE;
    }
    return TRUE;
}

UTEST(identity, active_token) {
    rex_result result = {0};
    wchar_t empty[] = L"";

    reset_state();
    run_case(L"", empty, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_EQ(1, lifecycle_calls);
    EXPECT_EQ(0, thread_calls);
    EXPECT_EQ(0, logon_calls);
    EXPECT_EQ(REX_STATUS_SUCCEEDED, result.status);
}

UTEST(identity, downlevel_principal) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    run_case(L"DOMAIN\\operator", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(1, lifecycle_calls);
    EXPECT_EQ(1, thread_calls);
    EXPECT_EQ(1, logon_calls);
    EXPECT_FALSE(impersonating);
    EXPECT_EQ(REX_STATUS_SUCCEEDED, result.status);
}

UTEST(identity, user_principal_name) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    expected_username = L"operator@example.com";
    run_case(L"operator@example.com", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(1, lifecycle_calls);
    EXPECT_EQ(1, thread_calls);
    EXPECT_EQ(1, logon_calls);
    EXPECT_FALSE(impersonating);
    EXPECT_EQ(REX_STATUS_SUCCEEDED, result.status);
}

UTEST(identity, invalid_principal) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    run_case(L"operator", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(0, lifecycle_calls);
    EXPECT_EQ(0, thread_calls);
    EXPECT_EQ(0, logon_calls);
    EXPECT_EQ(REX_PHASE_INPUT, result.phase);
}

UTEST(identity, logon_failure) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    logon_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(0, lifecycle_calls);
    EXPECT_EQ(0, thread_calls);
    EXPECT_EQ(1, logon_calls);
    EXPECT_EQ(REX_PHASE_IDENTITY, result.phase);
}

UTEST(identity, impersonation_failure) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    impersonation_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(0, lifecycle_calls);
    EXPECT_EQ(1, thread_calls);
    EXPECT_EQ(1, logon_calls);
    EXPECT_FALSE(impersonating);
    EXPECT_EQ(REX_PHASE_IDENTITY, result.phase);
}

UTEST(identity, revert_failure) {
    rex_result result = {0};
    wchar_t password[7] = L"secret";

    reset_state();
    expected_username = L"operator";
    expected_domain = L"DOMAIN";
    revert_succeeds = FALSE;
    run_case(L"DOMAIN\\operator", password, &result);

    EXPECT_TRUE(calls_valid);
    EXPECT_TRUE(password_cleared(password, 7));
    EXPECT_EQ(1, lifecycle_calls);
    EXPECT_EQ(1, thread_calls);
    EXPECT_EQ(1, logon_calls);
    EXPECT_TRUE(impersonating);
    EXPECT_EQ(REX_PHASE_IDENTITY_RESTORE, result.phase);
}

UTEST_MAIN()
