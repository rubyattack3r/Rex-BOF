#ifndef REX_H
#define REX_H

#include <windows.h>

#define REX_MAX_TARGET 255
#define REX_MAX_SELECTOR 64
#define REX_MAX_COMMAND 8191

typedef enum {
    REX_ACTION_EXECUTE = 0,
    REX_ACTION_PREFLIGHT
} rex_action;

typedef struct {
    rex_action action;
    const char *target;
    const char *selector;
    const wchar_t *values[3];
} rex_arguments;

typedef struct {
    const wchar_t *text;
} rex_command_config;

typedef struct {
    const wchar_t *name;
    const wchar_t *binary_path;
} rex_service_config;

typedef struct {
    const wchar_t *name;
    const wchar_t *executable;
    const wchar_t *arguments;
} rex_task_config;

typedef struct {
    const wchar_t *executable;
    const wchar_t *arguments;
} rex_shell_config;

typedef struct {
    const wchar_t *executable;
    const wchar_t *arguments;
    BOOL use_https;
    USHORT port;
} rex_wsman_config;

typedef struct {
    const wchar_t *name;
    const wchar_t *class_id;
    const wchar_t *dll_path;
} rex_provider_config;

typedef union {
    rex_command_config command;
    rex_service_config service;
    rex_task_config task;
    rex_shell_config shell;
    rex_wsman_config wsman;
    rex_provider_config provider;
} rex_config;

typedef struct {
    const char *target;
    const char *selector;
    rex_config config;
} rex_request;

typedef struct {
    void *private_data;
} rex_state;

typedef struct {
    const wchar_t *name;
    const wchar_t *original_binary_path;
    const wchar_t *applied_binary_path;
    DWORD original_start_type;
} rex_service_restore;

typedef struct {
    const wchar_t *name;
    const wchar_t *binary_path;
} rex_created_service_artifact;

typedef struct {
    const wchar_t *name;
    const wchar_t *executable;
} rex_task_artifact;

typedef struct {
    const wchar_t *name;
    const wchar_t *class_id;
    const wchar_t *dll_path;
} rex_provider_artifact;

typedef enum {
    REX_RECOVERY_NONE = 0,
    REX_RECOVERY_RESTORE_SERVICE,
    REX_RECOVERY_DELETE_SERVICE,
    REX_RECOVERY_DELETE_TASK,
    REX_RECOVERY_REMOVE_PROVIDER
} rex_recovery_kind;

typedef struct {
    rex_recovery_kind kind;
    union {
        rex_service_restore service;
        rex_created_service_artifact created_service;
        rex_task_artifact task;
        rex_provider_artifact provider;
    } value;
} rex_recovery;

typedef struct {
    DWORD process_id;
    BOOL has_process_id;
    rex_recovery recovery;
} rex_observation;

typedef enum {
    REX_STATUS_FAILED = 0,
    REX_STATUS_SUCCEEDED
} rex_result_status;

typedef enum {
    REX_DISPOSITION_NONE = 0,
    REX_DISPOSITION_VALIDATED,
    REX_DISPOSITION_DISPATCHED,
    REX_DISPOSITION_COMPLETED
} rex_disposition;

typedef enum {
    REX_PHASE_NONE = 0,
    REX_PHASE_INPUT,
    REX_PHASE_PREFLIGHT,
    REX_PHASE_EXECUTION
} rex_phase;

typedef struct {
    const char *technique;
    const char *target;
    rex_result_status status;
    rex_disposition disposition;
    rex_phase phase;
    const char *evidence_source;
    rex_observation observation;
    HRESULT error;
} rex_result;

typedef HRESULT (*rex_decode_fn)(const rex_arguments *, rex_request *);
typedef HRESULT (*rex_preflight_fn)(const rex_request *, rex_state *);
typedef HRESULT (*rex_execute_fn)(const rex_request *, rex_state *,
                                  rex_observation *);
typedef void (*rex_release_fn)(rex_state *);

typedef struct {
    const char *selector;
    const char *preflight_evidence_source;
    const char *execution_evidence_source;
    rex_disposition success_disposition;
    rex_decode_fn decode;
    rex_preflight_fn preflight;
    rex_execute_fn execute;
    rex_release_fn release;
} rex_technique;

extern const rex_technique rex_technique_descriptor;

SIZE_T rex_strlen(const char *value);
SIZE_T rex_wcslen(const wchar_t *value);
BOOL rex_streq(const char *left, const char *right);
BOOL rex_valid_target(const char *target);
BOOL rex_valid_selector(const char *selector);
void rex_zero(void *value, SIZE_T length);
void rex_copy(void *destination, const void *source, SIZE_T length);
void rex_result_write(const rex_result *result);

const rex_technique *rex_find_technique(const char *selector);

#endif
