#include "text.h"

#ifdef REX_TEXT_TEST
#define rex_WideCharToMultiByte WideCharToMultiByte
#define rex_GetProcessHeap GetProcessHeap
#define rex_HeapAlloc HeapAlloc
#define rex_HeapFree HeapFree
#else
DECLSPEC_IMPORT int WINAPI KERNEL32$WideCharToMultiByte(
    UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetProcessHeap(void);
DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);
#define rex_WideCharToMultiByte KERNEL32$WideCharToMultiByte
#define rex_GetProcessHeap KERNEL32$GetProcessHeap
#define rex_HeapAlloc KERNEL32$HeapAlloc
#define rex_HeapFree KERNEL32$HeapFree
#endif

static BOOL rex_text_safe(unsigned char value) {
    return value > 0x20 && value != 0x7f && value != '%' && value != '=';
}

static char rex_text_hex(unsigned char value) {
    return value < 10 ? (char)('0' + value) : (char)('A' + value - 10);
}

char *rex_text_encode_wide(const wchar_t *value) {
    HANDLE heap = rex_GetProcessHeap();
    char *utf8;
    char *encoded;
    SIZE_T encoded_length = 0;
    SIZE_T input;
    SIZE_T output = 0;
    int utf8_length;
    if (value == NULL) return NULL;
    utf8_length = rex_WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, NULL, 0, NULL, NULL);
    if (utf8_length <= 0) return NULL;
    utf8 = (char *)rex_HeapAlloc(heap, 0, (SIZE_T)utf8_length);
    if (utf8 == NULL) return NULL;
    if (rex_WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
            utf8, utf8_length, NULL, NULL) <= 0) {
        rex_HeapFree(heap, 0, utf8);
        return NULL;
    }
    for (input = 0; utf8[input] != '\0'; input++) {
        SIZE_T amount = rex_text_safe((unsigned char)utf8[input]) ? 1 : 3;
        if (encoded_length > (SIZE_T)-1 - amount) {
            rex_HeapFree(heap, 0, utf8);
            return NULL;
        }
        encoded_length += amount;
    }
    if (input == 1 && utf8[0] == '-') encoded_length = 3;
    if (encoded_length == (SIZE_T)-1) {
        rex_HeapFree(heap, 0, utf8);
        return NULL;
    }
    encoded = (char *)rex_HeapAlloc(heap, 0, encoded_length + 1);
    if (encoded == NULL) {
        rex_HeapFree(heap, 0, utf8);
        return NULL;
    }
    for (input = 0; utf8[input] != '\0'; input++) {
        unsigned char current = (unsigned char)utf8[input];
        BOOL encode = !rex_text_safe(current) ||
            (input == 0 && utf8[1] == '\0' && current == '-');
        if (encode) {
            encoded[output++] = '%';
            encoded[output++] = rex_text_hex((unsigned char)(current >> 4));
            encoded[output++] = rex_text_hex((unsigned char)(current & 0x0f));
        } else {
            encoded[output++] = (char)current;
        }
    }
    encoded[output] = '\0';
    rex_HeapFree(heap, 0, utf8);
    return encoded;
}

void rex_text_free(char *value) {
    if (value != NULL) rex_HeapFree(rex_GetProcessHeap(), 0, value);
}
