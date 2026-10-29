#!/bin/sh
set -eu

NM="${LLVM_NM:-llvm-nm}"
OBJDUMP="${LLVM_OBJDUMP:-llvm-objdump}"

verify_object() {
    object=$1
    format=$2
    entry=$3

    "$OBJDUMP" -f "$object" | grep -Fq "file format $format"
    "$NM" -g "$object" | grep -Eq " T ${entry}$"

    if "$NM" -u "$object" | grep -Ev \
        ' __imp__?(BeaconDataExtract|BeaconDataLength|BeaconDataParse|BeaconPrintf|toWideChar|[A-Z0-9]+\$[A-Za-z0-9_]+)(@[0-9]+)?$' \
        | grep -q .; then
        echo "unexpected or non-dllimport BOF imports in $object:" >&2
        "$NM" -u "$object" >&2
        exit 1
    fi

    if [ "$format" = coff-x86-64 ]; then
        unsupported=$(
            "$OBJDUMP" -r -h "$object" \
                | grep -E 'IMAGE_REL_AMD64_ADDR64|\.refptr' || true
        )
        if [ -n "$unsupported" ]; then
            echo "unsupported x64 Cobalt Strike relocations in $object:" >&2
            echo "$unsupported" >&2
            exit 1
        fi
    fi
}

for object in "$@"; do
    case "$object" in
        *.x64.o) verify_object "$object" coff-x86-64 go ;;
        *.x86.o) verify_object "$object" coff-i386 _go ;;
        *)
            echo "unknown BOF architecture: $object" >&2
            exit 1
            ;;
    esac
done

echo "verified $# Cobalt Strike BOF objects"
