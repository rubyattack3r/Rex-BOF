#!/usr/bin/env python3

import json
import os
import pathlib
import subprocess
import sys


SUPPORTED_ARCHES = {"amd64", "386"}


def fail(message: str) -> None:
    raise SystemExit(f"Sliver entrypoint check failed: {message}")


def defined_symbols(tool: str, path: pathlib.Path) -> set[str]:
    try:
        result = subprocess.run(
            [tool, "-g", "--defined-only", str(path)],
            check=True,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        fail(f"required tool is missing: {tool}")
    except subprocess.CalledProcessError as error:
        fail(f"{tool} could not inspect {path}: {error.stderr.strip()}")
    return {
        line.split()[-1]
        for line in result.stdout.splitlines()
        if line.split()
    }


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} <extension.json>")
    manifest_path = pathlib.Path(sys.argv[1])
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    nm = os.environ.get("LLVM_NM", "llvm-nm")
    checked: set[tuple[pathlib.Path, str]] = set()

    for command in manifest.get("commands", []):
        name = command.get("command_name", "<unnamed>")
        entrypoint = command.get("entrypoint")
        if not entrypoint:
            fail(f"{name}: missing entrypoint")
        for item in command.get("files", []):
            arch = item.get("arch")
            relative_path = item.get("path")
            if arch not in SUPPORTED_ARCHES or not relative_path:
                fail(f"{name}: invalid file binding")
            object_path = manifest_path.parent / relative_path
            if not object_path.is_file():
                fail(f"{name}: object is missing: {object_path}")
            binding = (object_path, entrypoint)
            if binding in checked:
                continue
            symbols = defined_symbols(nm, object_path)
            if entrypoint not in symbols:
                fail(
                    f"{name}: {object_path.name} does not define exact symbol "
                    f"{entrypoint!r}"
                )
            checked.add(binding)

    print(f"checked {len(checked)} Sliver entrypoint bindings")


if __name__ == "__main__":
    main()
