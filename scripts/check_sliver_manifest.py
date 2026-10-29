#!/usr/bin/env python3

import json
import pathlib
import re
import sys


EXPECTED = {
    "rex-wmi-provider": ("wmiprovider", "go_sliver_3", [
        ("provider-name", False), ("provider-clsid", False),
        ("remote-path", False),
    ]),
    "rex-wmi-service": ("wmiservice", "go_sliver_2", [
        ("service-name", False), ("remote-path", False),
    ]),
    "rex-wmi-exec": ("wmiexec", "go_sliver_1", [("command", False)]),
    "rex-wsman-exec": ("wsmanexec", "go_sliver_3", [
        ("program", False), ("arguments", True), ("transport", True),
    ]),
    "rex-task-sched-exec": ("taskschedexec", "go_sliver_3", [
        ("task-name", False), ("program", False), ("arguments", True),
    ]),
    "rex-dcom-ishell-dispatch2": ("dcomishelldispatch2", "go_sliver_2", [
        ("program", False), ("arguments", True),
    ]),
    "rex-scm-change-service": ("scmchangeservice", "go_sliver_2", [
        ("service-name", False), ("remote-path", False),
    ]),
    "rex-scm-create-service": ("scmcreateservice", "go_sliver_2", [
        ("service-name", False), ("remote-path", False),
    ]),
}


def fail(message: str) -> None:
    raise SystemExit(f"Sliver manifest check failed: {message}")


def main() -> None:
    if len(sys.argv) not in (2, 3):
        raise SystemExit(
            f"usage: {sys.argv[0]} <extension.json> [expected-version]"
        )
    path = pathlib.Path(sys.argv[1])
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("name") != "rex-bof":
        fail("name must be rex-bof")
    version = manifest.get("version", "")
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:[.-][0-9A-Za-z][0-9A-Za-z.-]*)?", version):
        fail(f"invalid version: {version!r}")
    if len(sys.argv) == 3 and version != sys.argv[2]:
        fail(f"version {version!r} does not match {sys.argv[2]!r}")

    commands = manifest.get("commands")
    if not isinstance(commands, list):
        fail("commands must be an array")
    by_name = {command.get("command_name"): command for command in commands}
    if len(by_name) != len(commands):
        fail("command names must be unique")
    if set(by_name) != set(EXPECTED):
        fail("commands do not match the eight stable Rex selectors")

    for name, (basename, entrypoint, value_args) in EXPECTED.items():
        command = by_name[name]
        if command.get("entrypoint") != entrypoint:
            fail(f"{name}: entrypoint must be {entrypoint}")
        if command.get("bof_executor") != "reflektor":
            fail(f"{name}: bof_executor must be reflektor")
        if command.get("depends_on") != "coff-loader":
            fail(f"{name}: depends_on must provide the coff-loader fallback")
        expected_files = {
            ("windows", "amd64", f"rex_{basename}.x64.o"),
            ("windows", "386", f"rex_{basename}.x86.o"),
        }
        actual_files = {
            (item.get("os"), item.get("arch"), item.get("path"))
            for item in command.get("files", [])
        }
        if actual_files != expected_files:
            fail(f"{name}: files do not match the x86/x64 objects")

        arguments = command.get("arguments", [])
        expected_count = 2 + len(value_args)
        if len(arguments) != expected_count:
            fail(f"{name}: expected {expected_count} arguments")
        action = arguments[0]
        if (action.get("name"), action.get("type"), action.get("optional"),
                action.get("default"), action.get("choices")) != (
                    "action", "string", True, "execute",
                    ["execute", "preflight"]):
            fail(f"{name}: invalid action argument")
        target = arguments[1]
        if (target.get("name"), target.get("type"), target.get("optional")) != (
                "target", "string", False):
            fail(f"{name}: invalid target argument")
        for argument, (expected_name, optional) in zip(arguments[2:], value_args):
            if (argument.get("name"), argument.get("type"),
                    argument.get("optional")) != (
                        expected_name, "wstring", optional):
                fail(f"{name}: invalid {expected_name} argument")
            if optional and argument.get("default") != "":
                fail(f"{name}: optional {expected_name} must default to empty")

    print(f"checked Sliver manifest with {len(commands)} commands")


if __name__ == "__main__":
    main()
