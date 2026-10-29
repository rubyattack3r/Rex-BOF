#include "rex.h"

const rex_technique *rex_find_technique(const char *selector) {
    if (selector != NULL && rex_streq(selector, rex_technique_descriptor.selector)) {
        return &rex_technique_descriptor;
    }
    return NULL;
}
