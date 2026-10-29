#include <windows.h>

#include "com.h"

#if defined(REX_COM_SECURITY_TEST)
HRESULT WINAPI rex_test_CoInitializeSecurity(PSECURITY_DESCRIPTOR, LONG,
                                             SOLE_AUTHENTICATION_SERVICE *,
                                             void *, DWORD, DWORD, void *,
                                             DWORD, void *);
#define REX_COINITIALIZESECURITY rex_test_CoInitializeSecurity
#else
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeSecurity(
    PSECURITY_DESCRIPTOR, LONG, SOLE_AUTHENTICATION_SERVICE *, void *, DWORD,
    DWORD, void *, DWORD, void *);
#define REX_COINITIALIZESECURITY OLE32$CoInitializeSecurity
#endif

HRESULT rex_com_enable_dynamic_cloaking(void) {
    HRESULT result = REX_COINITIALIZESECURITY(
        NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DYNAMIC_CLOAKING, NULL);

    /* A host may have initialized process security before loading the BOF. */
    return result == RPC_E_TOO_LATE ? S_OK : result;
}
