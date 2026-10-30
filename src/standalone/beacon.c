#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#include "beacon.h"

void BeaconPrintf(int type, char *format, ...) {
    FILE *stream = type == CALLBACK_ERROR ? stderr : stdout;
    va_list arguments;
    va_start(arguments, format);
    (void)vfprintf(stream, format, arguments);
    va_end(arguments);
    (void)fflush(stream);
}

BOOL toWideChar(char *source, wchar_t *destination, int max_bytes) {
    int capacity;
    int written;
    if (source == NULL || destination == NULL ||
        max_bytes < (int)sizeof(wchar_t)) {
        return FALSE;
    }
    capacity = max_bytes / (int)sizeof(wchar_t);
    written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1,
                                  destination, capacity);
    return written > 0;
}
