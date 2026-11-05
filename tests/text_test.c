#include <string.h>
#include <windows.h>

#include "text.h"
#include "utest.h"

typedef struct {
    const wchar_t *input;
    const char *expected;
} rex_text_case;

UTEST(text, encode_transcript_values) {
    static const rex_text_case cases[] = {
        {L"", ""},
        {L"-", "%2D"},
        {L"Windows Update", "Windows%20Update"},
        {L"name=value%done", "name%3Dvalue%25done"},
        {L"line\r\nnext\tvalue", "line%0D%0Anext%09value"},
        {L"C:\\Program Files\\rex.dll", "C:\\Program%20Files\\rex.dll"},
        {L"\x65E5\x672C", "\xE6\x97\xA5\xE6\x9C\xAC"},
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        char *actual = rex_text_encode_wide(cases[index].input);
        ASSERT_NE(NULL, actual);
        EXPECT_STREQ(cases[index].expected, actual);
        rex_text_free(actual);
    }
}

UTEST(text, reject_invalid_input) {
    wchar_t invalid[] = {(wchar_t)0xd800, L'\0'};

    EXPECT_EQ(NULL, rex_text_encode_wide(NULL));
    EXPECT_EQ(NULL, rex_text_encode_wide(invalid));
}

UTEST_MAIN()
