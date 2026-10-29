#!/bin/sh
set -eu

if [ "$#" -ne 1 ] || ! printf '%s\n' "$1" |
    grep -Eq '^v[0-9]+\.[0-9]+\.[0-9]+([.-][0-9A-Za-z][0-9A-Za-z.-]*)?$'; then
    echo "usage: $0 <version, for example v0.1.0>" >&2
    exit 2
fi

VERSION=$1
PROJECT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
RELEASE_DIR=${RELEASE_DIR:-"$PROJECT_DIR/release"}
PACKAGE_NAME="Rex-BOF-$VERSION"
ARCHIVE="$RELEASE_DIR/$PACKAGE_NAME.zip"
CHECKSUM="$ARCHIVE.sha256"
WORK_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/rex-bof-release.XXXXXX")
PACKAGE_ROOT="$WORK_ROOT/$PACKAGE_NAME"
trap 'rm -rf "$WORK_ROOT"' EXIT HUP INT TERM

for required in \
    "$PROJECT_DIR/integrations/cna/rex.cna" \
    "$PROJECT_DIR/README.md" \
    "$PROJECT_DIR/CHANGELOG.md" \
    "$PROJECT_DIR/LICENSE" \
    "$PROJECT_DIR/ACCEPTABLE_USE_POLICY.md" \
    "$PROJECT_DIR/docs/index.md"; do
    if [ ! -f "$required" ]; then
        echo "required release input is missing: $required" >&2
        exit 1
    fi
done

for object in "$PROJECT_DIR"/integrations/cna/rex_*.x64.o \
    "$PROJECT_DIR"/integrations/cna/rex_*.x86.o; do
    if [ ! -f "$object" ]; then
        echo "required release object is missing: $object" >&2
        exit 1
    fi
done

mkdir -p "$PACKAGE_ROOT" "$RELEASE_DIR"
cp "$PROJECT_DIR/integrations/cna/rex.cna" "$PACKAGE_ROOT/"
cp "$PROJECT_DIR"/integrations/cna/rex_*.x64.o "$PACKAGE_ROOT/"
cp "$PROJECT_DIR"/integrations/cna/rex_*.x86.o "$PACKAGE_ROOT/"
cp "$PROJECT_DIR/README.md" "$PACKAGE_ROOT/"
cp "$PROJECT_DIR/CHANGELOG.md" "$PACKAGE_ROOT/"
cp "$PROJECT_DIR/LICENSE" "$PACKAGE_ROOT/"
cp "$PROJECT_DIR/ACCEPTABLE_USE_POLICY.md" "$PACKAGE_ROOT/"
cp -R "$PROJECT_DIR/docs" "$PACKAGE_ROOT/"
printf '%s\n' "$VERSION" > "$PACKAGE_ROOT/VERSION.txt"

find "$PACKAGE_ROOT" -exec touch -t 198001010000 {} +
(
    cd "$WORK_ROOT"
    LC_ALL=C find "$PACKAGE_NAME" -print | LC_ALL=C sort |
        zip -X -q "$WORK_ROOT/$PACKAGE_NAME.zip" -@
)
mv "$WORK_ROOT/$PACKAGE_NAME.zip" "$ARCHIVE"

if command -v sha256sum >/dev/null 2>&1; then
    digest=$(sha256sum "$ARCHIVE" | awk '{print $1}')
else
    digest=$(shasum -a 256 "$ARCHIVE" | awk '{print $1}')
fi
printf '%s  %s\n' "$digest" "$PACKAGE_NAME.zip" > "$WORK_ROOT/checksum"
mv "$WORK_ROOT/checksum" "$CHECKSUM"

sh "$PROJECT_DIR/scripts/verify-release.sh" "$ARCHIVE" "$CHECKSUM"
printf 'created release package: %s\n' "$ARCHIVE"
printf 'created release checksum: %s\n' "$CHECKSUM"
