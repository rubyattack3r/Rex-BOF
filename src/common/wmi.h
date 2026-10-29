#ifndef REX_WMI_H
#define REX_WMI_H

#include <windows.h>

typedef struct rex_wmi_session rex_wmi_session;

HRESULT rex_wmi_open(const char *target, rex_wmi_session **session);
HRESULT rex_wmi_open_namespace(const char *target, const wchar_t *namespace_name,
                               rex_wmi_session **session);
HRESULT rex_wmi_process_create(rex_wmi_session *session, const wchar_t *command,
                               DWORD *process_id);
HRESULT rex_wmi_service_change(rex_wmi_session *session, const wchar_t *name,
                               const wchar_t *binary_path);
HRESULT rex_wmi_service_start(rex_wmi_session *session, const wchar_t *name);
HRESULT rex_wmi_provider_exists(rex_wmi_session *, const wchar_t *, BOOL *);
HRESULT rex_wmi_provider_register(rex_wmi_session *, const wchar_t *,
                                  const wchar_t *);
HRESULT rex_wmi_provider_delete(rex_wmi_session *, const wchar_t *);
HRESULT rex_wmi_provider_load(rex_wmi_session *, const wchar_t *);
HRESULT rex_wmi_provider_unload(rex_wmi_session *, const wchar_t *);
void rex_wmi_close(rex_wmi_session *session);

#endif
