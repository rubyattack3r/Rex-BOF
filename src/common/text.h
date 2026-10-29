#ifndef REX_TEXT_H
#define REX_TEXT_H

#include <windows.h>

/* Encodes whitespace, percent signs, equals signs, and control bytes as %XX. */
char *rex_text_encode_wide(const wchar_t *value);
void rex_text_free(char *value);

#endif
