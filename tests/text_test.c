#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "text.h"

typedef struct {
    const wchar_t *input;
    const char *expected;
} rex_text_case;

int main(void) {
    static const rex_text_case cases[] = {
        {L"", ""},
        {L"-", "%2D"},
        {L"Windows Update", "Windows%20Update"},
        {L"name=value%done", "name%3Dvalue%25done"},
        {L"line\r\nnext\tvalue", "line%0D%0Anext%09value"},
        {L"C:\\Program Files\\rex.dll", "C:\\Program%20Files\\rex.dll"},
        {L"\x65E5\x672C", "\xE6\x97\xA5\xE6\x9C\xAC"},
    };
    wchar_t invalid[] = {(wchar_t)0xd800, L'\0'};
    size_t index;
    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        char *actual = rex_text_encode_wide(cases[index].input);
        if (actual == NULL || strcmp(actual, cases[index].expected) != 0) {
            fprintf(stderr, "case %zu: expected [%s], got [%s]\n", index,
                    cases[index].expected, actual == NULL ? "<null>" : actual);
            rex_text_free(actual);
            return 1;
        }
        rex_text_free(actual);
    }
    if (rex_text_encode_wide(NULL) != NULL ||
        rex_text_encode_wide(invalid) != NULL) {
        fputs("invalid input was accepted\n", stderr);
        return 1;
    }
    return 0;
}
