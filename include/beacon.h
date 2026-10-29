#ifndef REX_BEACON_H
#define REX_BEACON_H

#include <windows.h>

typedef struct {
    char *original;
    char *buffer;
    int length;
    int size;
} datap;

#define CALLBACK_OUTPUT 0x00
#define CALLBACK_ERROR 0x0d
#define CALLBACK_OUTPUT_UTF8 0x20

DECLSPEC_IMPORT void BeaconDataParse(datap *parser, char *buffer, int size);
DECLSPEC_IMPORT char *BeaconDataExtract(datap *parser, int *size);
DECLSPEC_IMPORT int BeaconDataLength(datap *parser);
DECLSPEC_IMPORT void BeaconPrintf(int type, char *format, ...);
DECLSPEC_IMPORT BOOL toWideChar(char *source, wchar_t *destination, int max_bytes);

#endif
