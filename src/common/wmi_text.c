#include "wmi_text.h"

static int rex_wmi_class_name_valid(const wchar_t *value) {
    size_t index;
    if (value == NULL || value[0] == L'\0')
        return 0;
    for (index = 0; value[index] != L'\0'; index++) {
        wchar_t current = value[index];
        if (!((current >= L'a' && current <= L'z') ||
              (current >= L'A' && current <= L'Z') ||
              (current >= L'0' && current <= L'9') || current == L'_')) {
            return 0;
        }
    }
    return 1;
}

int rex_wmi_name_valid(const wchar_t *value) {
    size_t index;
    if (value == NULL || value[0] == L'\0')
        return 0;
    for (index = 0; value[index] != L'\0'; index++) {
        unsigned long current = (unsigned long)value[index];
        if (current < 0x20 || (current >= 0x7f && current <= 0x9f))
            return 0;
        if (sizeof(wchar_t) == 2) {
            if (current >= 0xd800 && current <= 0xdbff) {
                unsigned long next = (unsigned long)value[index + 1];
                if (next < 0xdc00 || next > 0xdfff)
                    return 0;
                index++;
            } else if (current >= 0xdc00 && current <= 0xdfff) {
                return 0;
            }
        } else if (current > 0x10ffff ||
                   (current >= 0xd800 && current <= 0xdfff)) {
            return 0;
        }
    }
    return 1;
}

size_t rex_wmi_named_path_capacity(const wchar_t *class_name,
                                   const wchar_t *name, wchar_t quote) {
    size_t class_length = 0;
    size_t escaped_length = 0;
    size_t index;
    if (!rex_wmi_class_name_valid(class_name) || !rex_wmi_name_valid(name) ||
        (quote != L'\'' && quote != L'"')) {
        return 0;
    }
    while (class_name[class_length] != L'\0')
        class_length++;
    for (index = 0; name[index] != L'\0'; index++) {
        size_t amount = name[index] == L'\\' || name[index] == quote ? 2 : 1;
        if (escaped_length > (size_t)-1 - amount)
            return 0;
        escaped_length += amount;
    }
    if (escaped_length > (size_t)-1 - 9 ||
        class_length > (size_t)-1 - escaped_length - 9)
        return 0;
    return class_length + escaped_length + 9;
}

int rex_wmi_named_path_write(const wchar_t *class_name, const wchar_t *name,
                             wchar_t quote, wchar_t *output, size_t capacity) {
    static const wchar_t key[] = L".Name=";
    size_t required = rex_wmi_named_path_capacity(class_name, name, quote);
    size_t input = 0;
    size_t written = 0;
    if (required == 0 || output == NULL || capacity < required)
        return 0;
    while (class_name[input] != L'\0')
        output[written++] = class_name[input++];
    input = 0;
    while (key[input] != L'\0')
        output[written++] = key[input++];
    output[written++] = quote;
    for (input = 0; name[input] != L'\0'; input++) {
        if (name[input] == L'\\' || name[input] == quote)
            output[written++] = L'\\';
        output[written++] = name[input];
    }
    output[written++] = quote;
    output[written] = L'\0';
    return written + 1 == required;
}
