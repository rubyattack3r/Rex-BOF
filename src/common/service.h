#ifndef REX_SERVICE_H
#define REX_SERVICE_H

#include <windows.h>

typedef struct rex_service_snapshot rex_service_snapshot;

HRESULT rex_service_snapshot_open(const char *, const wchar_t *, DWORD,
                                  rex_service_snapshot **);
HRESULT rex_service_change_native(rex_service_snapshot *, const wchar_t *);
HRESULT rex_service_start_native(rex_service_snapshot *);
const wchar_t *rex_service_original_binary_path(const rex_service_snapshot *);
DWORD rex_service_original_start_type(const rex_service_snapshot *);
void rex_service_snapshot_close(rex_service_snapshot *);

typedef struct rex_created_service rex_created_service;
HRESULT rex_service_create_preflight(const char *, const wchar_t *,
                                     rex_created_service **);
HRESULT rex_service_create(rex_created_service *, const wchar_t *);
HRESULT rex_service_created_start(rex_created_service *);
void rex_service_created_close(rex_created_service *);

#endif
