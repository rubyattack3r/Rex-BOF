#include <stdint.h>
#include <wchar.h>

#include "utest.h"
#include "wmi_text.h"

static int wide_equal(const wchar_t *left, const wchar_t *right) {
    size_t index = 0;
    while (left[index] != L'\0' && right[index] != L'\0') {
        if (left[index] != right[index])
            return 0;
        index++;
    }
    return left[index] == right[index];
}

static int path_matches(const wchar_t *name, wchar_t quote,
                        const wchar_t *expected) {
    wchar_t output[128];
    size_t required = rex_wmi_named_path_capacity(L"Example", name, quote);
    if (required == 0 || required > sizeof(output) / sizeof(output[0]) ||
        !rex_wmi_named_path_write(L"Example", name, quote, output, required) ||
        !wide_equal(output, expected)) {
        return 0;
    }
    return !rex_wmi_named_path_write(L"Example", name, quote, output,
                                     required - 1);
}

UTEST(wmi_text, validate_names) {
    static const wchar_t mixed[] =
        L"Rex-\x00e9-e\x0301-\x65e5\x672c-\x041f\x0440\x0438\x0432"
        L"\x0435\x0442-\x0645\x0631\x062d\x0628\x0627";
    static const wchar_t escaped[] = L"A\\B\"C'D";
    static const wchar_t control[] = {L'A', L'\n', L'B', L'\0'};
    static const wchar_t high_surrogate[] = {(wchar_t)0xd83d, L'\0'};
    static const wchar_t low_surrogate[] = {(wchar_t)0xde00, L'\0'};
#if WCHAR_MAX <= 0xffff
    static const wchar_t supplementary[] = {(wchar_t)0xd83d, (wchar_t)0xde00,
                                            L'\0'};
#else
    static const wchar_t supplementary[] = {(wchar_t)0x1f600, L'\0'};
#endif

    EXPECT_TRUE(rex_wmi_name_valid(mixed));
    EXPECT_TRUE(rex_wmi_name_valid(escaped));
    EXPECT_TRUE(rex_wmi_name_valid(supplementary));
    EXPECT_FALSE(rex_wmi_name_valid(L""));
    EXPECT_FALSE(rex_wmi_name_valid(control));
    EXPECT_FALSE(rex_wmi_name_valid(high_surrogate));
    EXPECT_FALSE(rex_wmi_name_valid(low_surrogate));
}

UTEST(wmi_text, escape_named_paths) {
    static const wchar_t escaped[] = L"A\\B\"C'D";

    EXPECT_TRUE(path_matches(escaped, L'"', L"Example.Name=\"A\\\\B\\\"C'D\""));
    EXPECT_TRUE(path_matches(escaped, L'\'', L"Example.Name='A\\\\B\"C\\'D'"));
}

UTEST(wmi_text, reject_invalid_paths) {
    static const wchar_t control[] = {L'A', L'\n', L'B', L'\0'};

    EXPECT_EQ(0, rex_wmi_named_path_capacity(L"Bad.Class", L"value", L'"'));
    EXPECT_EQ(0, rex_wmi_named_path_capacity(L"Example", control, L'"'));
    EXPECT_EQ(0, rex_wmi_named_path_capacity(L"Example", L"value", L'`'));
}

UTEST_MAIN()
