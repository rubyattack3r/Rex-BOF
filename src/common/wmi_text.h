#ifndef REX_WMI_TEXT_H
#define REX_WMI_TEXT_H

#include <stddef.h>

int rex_wmi_name_valid(const wchar_t *value);
size_t rex_wmi_named_path_capacity(const wchar_t *class_name,
                                   const wchar_t *name, wchar_t quote);
int rex_wmi_named_path_write(const wchar_t *class_name, const wchar_t *name,
                             wchar_t quote, wchar_t *output, size_t capacity);

#endif
