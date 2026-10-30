#include "rex.h"

#if defined(REX_STANDALONE)
typedef void (*rex_loader_fn)(rex_technique *);

void rex_wmi_provider_load_technique(rex_technique *);
void rex_wmi_instance_provider_load_technique(rex_technique *);
void rex_wmi_method_provider_load_technique(rex_technique *);
void rex_wmi_event_subscription_load_technique(rex_technique *);
void rex_wmi_service_load_technique(rex_technique *);
void rex_wmiexec_load_technique(rex_technique *);
void rex_wsman_load_technique(rex_technique *);
void rex_tasksched_load_technique(rex_technique *);
void rex_dcom_shell_load_technique(rex_technique *);
void rex_scm_change_load_technique(rex_technique *);
void rex_scm_create_load_technique(rex_technique *);
void rex_scm_command_load_technique(rex_technique *);

static const rex_loader_fn rex_loaders[] = {
    rex_wmi_provider_load_technique,
    rex_wmi_instance_provider_load_technique,
    rex_wmi_method_provider_load_technique,
    rex_wmi_event_subscription_load_technique,
    rex_wmi_service_load_technique,
    rex_wmiexec_load_technique,
    rex_wsman_load_technique,
    rex_tasksched_load_technique,
    rex_dcom_shell_load_technique,
    rex_scm_change_load_technique,
    rex_scm_create_load_technique,
    rex_scm_command_load_technique,
};
#endif

BOOL rex_find_technique(const char *selector, rex_technique *technique) {
#if defined(REX_STANDALONE)
    SIZE_T index;
#endif
    if (selector == NULL || technique == NULL) {
        return FALSE;
    }
#if defined(REX_STANDALONE)
    for (index = 0; index < sizeof(rex_loaders) / sizeof(rex_loaders[0]);
         index++) {
        rex_zero(technique, sizeof(*technique));
        rex_loaders[index](technique);
        if (rex_streq(selector, technique->selector))
            return TRUE;
    }
#else
    rex_zero(technique, sizeof(*technique));
    rex_load_technique(technique);
    if (rex_streq(selector, technique->selector)) {
        return TRUE;
    }
#endif
    rex_zero(technique, sizeof(*technique));
    return FALSE;
}
