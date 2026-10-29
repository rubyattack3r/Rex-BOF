#include "beacon.h"
#include "rex.h"
#include "text.h"

static const char *rex_value(const char *value) {
    return value == NULL || value[0] == '\0' ? "-" : value;
}

static const char *rex_disposition_name(rex_disposition disposition) {
    if (disposition == REX_DISPOSITION_VALIDATED)
        return "validated";
    if (disposition == REX_DISPOSITION_DISPATCHED)
        return "dispatched";
    if (disposition == REX_DISPOSITION_COMPLETED)
        return "completed";
    return "none";
}

static const char *rex_phase_name(rex_phase phase) {
    if (phase == REX_PHASE_INPUT)
        return "input";
    if (phase == REX_PHASE_PREFLIGHT)
        return "preflight";
    if (phase == REX_PHASE_EXECUTION)
        return "execution";
    return "none";
}

static void rex_recovery_write(const rex_recovery *recovery,
                               const char *target) {
    char *name;
    char *original;
    char *applied;
    char *class_id;
    char *dll_path;
    if (recovery == NULL || recovery->kind == REX_RECOVERY_NONE)
        return;
    if (recovery->kind == REX_RECOVERY_RESTORE_SERVICE) {
        name = rex_text_encode_wide(recovery->value.service.name);
        original =
            rex_text_encode_wide(recovery->value.service.original_binary_path);
        applied =
            rex_text_encode_wide(recovery->value.service.applied_binary_path);
        BeaconPrintf(
            CALLBACK_OUTPUT_UTF8,
            "recovery target=%s action=restore_service service=%s "
            "original_binary=%s applied_binary=%s original_start_type=%lu "
            "original_state=stopped\n",
            rex_value(target), rex_value(name), rex_value(original),
            rex_value(applied),
            (unsigned long)recovery->value.service.original_start_type);
        rex_text_free(applied);
        rex_text_free(original);
        rex_text_free(name);
        return;
    }
    if (recovery->kind == REX_RECOVERY_DELETE_SERVICE) {
        name = rex_text_encode_wide(recovery->value.created_service.name);
        applied =
            rex_text_encode_wide(recovery->value.created_service.binary_path);
        BeaconPrintf(
            CALLBACK_OUTPUT_UTF8,
            "recovery target=%s action=delete_service service=%s binary=%s\n",
            rex_value(target), rex_value(name), rex_value(applied));
        rex_text_free(applied);
        rex_text_free(name);
        return;
    }
    if (recovery->kind == REX_RECOVERY_DELETE_TASK) {
        name = rex_text_encode_wide(recovery->value.task.name);
        applied = rex_text_encode_wide(recovery->value.task.executable);
        BeaconPrintf(
            CALLBACK_OUTPUT_UTF8,
            "recovery target=%s action=delete_task task=%s executable=%s\n",
            rex_value(target), rex_value(name), rex_value(applied));
        rex_text_free(applied);
        rex_text_free(name);
        return;
    }
    if (recovery->kind == REX_RECOVERY_REMOVE_PROVIDER) {
        name = rex_text_encode_wide(recovery->value.provider.name);
        class_id = rex_text_encode_wide(recovery->value.provider.class_id);
        dll_path = rex_text_encode_wide(recovery->value.provider.dll_path);
        BeaconPrintf(CALLBACK_OUTPUT_UTF8,
                     "recovery target=%s action=remove_wmi_provider "
                     "provider=%s clsid=%s dll=%s\n",
                     rex_value(target), rex_value(name), rex_value(class_id),
                     rex_value(dll_path));
        rex_text_free(dll_path);
        rex_text_free(class_id);
        rex_text_free(name);
        return;
    }
    if (recovery->kind == REX_RECOVERY_REMOVE_WMI_SUBSCRIPTION) {
        name = rex_text_encode_wide(recovery->value.subscription.name);
        BeaconPrintf(CALLBACK_OUTPUT_UTF8,
                     "recovery target=%s action=remove_wmi_subscription "
                     "subscription=%s\n",
                     rex_value(target), rex_value(name));
        rex_text_free(name);
    }
}

void rex_result_write(const rex_result *result) {
    if (result == NULL)
        return;
    if (result->status == REX_STATUS_FAILED) {
        BeaconPrintf(
            CALLBACK_ERROR,
            "technique=%s target=%s status=failed phase=%s hresult=0x%08lx\n",
            rex_value(result->technique), rex_value(result->target),
            rex_phase_name(result->phase), (unsigned long)result->error);
        rex_recovery_write(&result->observation.recovery, result->target);
        return;
    }
    if (result->observation.has_process_id) {
        BeaconPrintf(CALLBACK_OUTPUT_UTF8,
                     "technique=%s target=%s status=succeeded disposition=%s "
                     "source=%s pid=%lu\n",
                     rex_value(result->technique), rex_value(result->target),
                     rex_disposition_name(result->disposition),
                     rex_value(result->evidence_source),
                     (unsigned long)result->observation.process_id);
        rex_recovery_write(&result->observation.recovery, result->target);
        return;
    }
    BeaconPrintf(
        CALLBACK_OUTPUT_UTF8,
        "technique=%s target=%s status=succeeded disposition=%s source=%s\n",
        rex_value(result->technique), rex_value(result->target),
        rex_disposition_name(result->disposition),
        rex_value(result->evidence_source));
    rex_recovery_write(&result->observation.recovery, result->target);
}
