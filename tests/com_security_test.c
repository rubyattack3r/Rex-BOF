#include <windows.h>

#include "com.h"
#include "utest.h"

static HRESULT configured_result;
static BOOL arguments_valid;

HRESULT WINAPI rex_test_CoInitializeSecurity(
    PSECURITY_DESCRIPTOR descriptor, LONG service_count,
    SOLE_AUTHENTICATION_SERVICE *services, void *reserved1, DWORD authn_level,
    DWORD impersonation_level, void *authentication_list, DWORD capabilities,
    void *reserved3) {
    if (descriptor != NULL || service_count != -1 || services != NULL ||
        reserved1 != NULL || authn_level != RPC_C_AUTHN_LEVEL_PKT_PRIVACY ||
        impersonation_level != RPC_C_IMP_LEVEL_IMPERSONATE ||
        authentication_list != NULL || capabilities != EOAC_DYNAMIC_CLOAKING ||
        reserved3 != NULL) {
        arguments_valid = FALSE;
    }
    return configured_result;
}

static HRESULT enable_with(HRESULT configured) {
    configured_result = configured;
    arguments_valid = TRUE;
    return rex_com_enable_dynamic_cloaking();
}

UTEST(com_security, success) {
    EXPECT_EQ(S_OK, enable_with(S_OK));
    EXPECT_TRUE(arguments_valid);
}

UTEST(com_security, already_initialized) {
    EXPECT_EQ(S_FALSE, enable_with(S_FALSE));
    EXPECT_TRUE(arguments_valid);
}

UTEST(com_security, initialized_too_late) {
    EXPECT_EQ(S_OK, enable_with(RPC_E_TOO_LATE));
    EXPECT_TRUE(arguments_valid);
}

UTEST(com_security, failure) {
    EXPECT_EQ(E_ACCESSDENIED, enable_with(E_ACCESSDENIED));
    EXPECT_TRUE(arguments_valid);
}

UTEST_MAIN()
