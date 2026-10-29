#include <windows.h>

#include "rex.h"

void rex_zero(void *value, SIZE_T length) {
    BYTE *bytes = (BYTE *)value;
    SIZE_T index;
    for (index = 0; index < length; index++)
        bytes[index] = 0;
}

void rex_copy(void *destination, const void *source, SIZE_T length) {
    BYTE *output = (BYTE *)destination;
    const BYTE *input = (const BYTE *)source;
    SIZE_T index;
    for (index = 0; index < length; index++)
        output[index] = input[index];
}

SIZE_T rex_strlen(const char *value) {
    SIZE_T length = 0;
    if (value == NULL) {
        return 0;
    }
    while (value[length] != '\0') {
        length++;
    }
    return length;
}

SIZE_T rex_wcslen(const wchar_t *value) {
    SIZE_T length = 0;
    if (value == NULL) {
        return 0;
    }
    while (value[length] != L'\0') {
        length++;
    }
    return length;
}

BOOL rex_streq(const char *left, const char *right) {
    SIZE_T index = 0;
    if (left == NULL || right == NULL) {
        return FALSE;
    }
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return FALSE;
        }
        index++;
    }
    return left[index] == right[index];
}

BOOL rex_valid_target(const char *target) {
    SIZE_T index;
    SIZE_T length = rex_strlen(target);
    if (length == 0 || length > REX_MAX_TARGET) {
        return FALSE;
    }
    for (index = 0; index < length; index++) {
        char value = target[index];
        if ((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
            (value >= '0' && value <= '9') || value == '.' || value == '-' ||
            value == '_' || value == ':') {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL rex_valid_selector(const char *selector) {
    SIZE_T index;
    SIZE_T length = rex_strlen(selector);
    if (length == 0 || length > REX_MAX_SELECTOR) {
        return FALSE;
    }
    for (index = 0; index < length; index++) {
        char value = selector[index];
        if ((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
            (value >= '0' && value <= '9') || value == '-') {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}
