#!/bin/sh
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: $0 <sliver-release.zip> [sliver-release.zip.sha256]" >&2
    exit 2
fi

ARCHIVE=$1
CHECKSUM=${2:-"$ARCHIVE.sha256"}
ARCHIVE_DIR=$(CDPATH='' cd -- "$(dirname -- "$ARCHIVE")" && pwd)
ARCHIVE_BASE=$(basename -- "$ARCHIVE")
CHECKSUM_BASE=$(basename -- "$CHECKSUM")
PACKAGE_NAME=${ARCHIVE_BASE%.zip}
MANIFEST_VERSION=${PACKAGE_NAME#Rex-BOF-Sliver-v}
PROJECT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
ACTUAL=$(mktemp "${TMPDIR:-/tmp}/rex-bof-sliver-actual.XXXXXX")
EXPECTED=$(mktemp "${TMPDIR:-/tmp}/rex-bof-sliver-expected.XXXXXX")
EXTRACTED=$(mktemp -d "${TMPDIR:-/tmp}/rex-bof-sliver-extracted.XXXXXX")
trap 'rm -f "$ACTUAL" "$EXPECTED"; rm -rf "$EXTRACTED"' EXIT HUP INT TERM

test -f "$ARCHIVE"
test -f "$CHECKSUM"
unzip -tq "$ARCHIVE" >/dev/null
unzip -Z1 "$ARCHIVE" | LC_ALL=C sort > "$ACTUAL"
{
    printf '%s\n' \
        "$PACKAGE_NAME/" \
        "$PACKAGE_NAME/ACCEPTABLE_USE_POLICY.md" \
        "$PACKAGE_NAME/CHANGELOG.md" \
        "$PACKAGE_NAME/LICENSE" \
        "$PACKAGE_NAME/README.md" \
        "$PACKAGE_NAME/VERSION.txt" \
        "$PACKAGE_NAME/extension.json"
    for technique in wmiprovider wmieventsubscription wmiservice wmiexec \
        wsmanexec taskschedexec dcomishelldispatch2 \
        scmchangeservice scmcreateservice scmcommand; do
        printf '%s\n' \
            "$PACKAGE_NAME/rex_$technique.x64.o" \
            "$PACKAGE_NAME/rex_$technique.x86.o"
    done
    (
        cd "$PROJECT_DIR"
        find docs -type d -print |
            sed -e "s|^|$PACKAGE_NAME/|" -e 's|$|/|'
        find docs -type f -print | sed "s|^|$PACKAGE_NAME/|"
    )
} | LC_ALL=C sort > "$EXPECTED"
diff -u "$EXPECTED" "$ACTUAL"

unzip -q "$ARCHIVE" -d "$EXTRACTED"
python3 "$PROJECT_DIR/scripts/check_docs.py" "$EXTRACTED/$PACKAGE_NAME"
python3 "$PROJECT_DIR/scripts/check_sliver_manifest.py" \
    "$EXTRACTED/$PACKAGE_NAME/extension.json" "$MANIFEST_VERSION"
python3 "$PROJECT_DIR/scripts/check_sliver_entrypoints.py" \
    "$EXTRACTED/$PACKAGE_NAME/extension.json"

if command -v sha256sum >/dev/null 2>&1; then
    (cd "$ARCHIVE_DIR" && sha256sum -c "$CHECKSUM_BASE")
else
    (cd "$ARCHIVE_DIR" && shasum -a 256 -c "$CHECKSUM_BASE")
fi

printf 'verified Sliver release package: %s\n' "$ARCHIVE"
