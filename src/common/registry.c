#include "rex.h"

BOOL rex_find_technique(const char *selector, rex_technique *technique) {
    if (selector == NULL || technique == NULL) {
        return FALSE;
    }
    rex_zero(technique, sizeof(*technique));
    rex_load_technique(technique);
    if (rex_streq(selector, technique->selector)) {
        return TRUE;
    }
    rex_zero(technique, sizeof(*technique));
    return FALSE;
}
