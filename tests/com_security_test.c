#include <stdio.h>
#include <windows.h>

#include "com.h"

static HRESULT configured_result;
static unsigned int failures;

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
        failures++;
    }
    return configured_result;
}

static void expect_result(HRESULT configured, HRESULT expected) {
    HRESULT actual;
    configured_result = configured;
    actual = rex_com_enable_dynamic_cloaking();
    if (actual != expected) {
        failures++;
    }
}

int main(void) {
    expect_result(S_OK, S_OK);
    expect_result(S_FALSE, S_FALSE);
    expect_result(RPC_E_TOO_LATE, S_OK);
    expect_result(E_ACCESSDENIED, E_ACCESSDENIED);
    if (failures != 0) {
        fprintf(stderr, "COM security tests failed: %u\n", failures);
        return 1;
    }
    puts("COM security tests passed");
    return 0;
}
