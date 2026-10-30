#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <rex.x64.exe> <rex.x86.exe>" >&2
    exit 2
fi

LLVM_OBJDUMP=${LLVM_OBJDUMP:-llvm-objdump}
X64=$1
X86=$2

test -f "$X64"
test -f "$X86"

if ! "$LLVM_OBJDUMP" -f "$X64" | grep -q 'file format coff-x86-64'; then
    echo "standalone x64 executable has the wrong PE architecture: $X64" >&2
    exit 1
fi
if ! "$LLVM_OBJDUMP" -f "$X86" | grep -q 'file format coff-i386'; then
    echo "standalone x86 executable has the wrong PE architecture: $X86" >&2
    exit 1
fi
if ! "$LLVM_OBJDUMP" -p "$X64" | grep -Eq 'AddressOfEntryPoint +0*[1-9a-fA-F]'; then
    echo "standalone x64 executable has no entry point: $X64" >&2
    exit 1
fi
if ! "$LLVM_OBJDUMP" -p "$X86" | grep -Eq 'AddressOfEntryPoint +0*[1-9a-fA-F]'; then
    echo "standalone x86 executable has no entry point: $X86" >&2
    exit 1
fi

printf 'verified x64 and x86 standalone PE executables\n'
