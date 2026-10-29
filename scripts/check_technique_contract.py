#!/usr/bin/env python3
"""Verify public technique names stay aligned across native and integrations."""

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "wmiprovider": "wmi-provider",
    "wmiservice": "wmi-service",
    "wmiexec": "wmi-exec",
    "wsmanexec": "wsman-exec",
    "taskschedexec": "task-sched-exec",
    "dcomishelldispatch2": "dcom-ishell-dispatch2",
    "scmchangeservice": "scm-change-service",
    "scmcreateservice": "scm-create-service",
}


def fail(message: str) -> None:
    raise SystemExit(f"technique contract check failed: {message}")


for source_name, selector in EXPECTED.items():
    source = (ROOT / "src" / "techniques" / f"{source_name}.c").read_text()
    descriptor = re.compile(
        r"const rex_technique rex_technique_descriptor\s*=\s*\{\s*"
        + re.escape(f'"{selector}"')
        + r"\s*,"
    )
    if descriptor.search(source) is None:
        fail(f"{source_name}.c does not use selector {selector}")

cna = (ROOT / "integrations" / "cna" / "rex.cna").read_text()
cna_known = re.search(
    r"sub rex_known_technique \{(?P<body>.*?)\n\}\n\nsub rex_object_name",
    cna,
    re.DOTALL,
)
if cna_known is None:
    fail("could not find rex_known_technique")
cna_names = set(re.findall(r'\$1 eq "([^"]+)"', cna_known.group("body")))
expected_names = set(EXPECTED.values())
if cna_names != expected_names:
    fail(f"CNA selectors are {sorted(cna_names)!r}, expected {sorted(expected_names)!r}")
if "rex_internal_selector" in cna:
    fail("CNA still contains an internal selector translation")
if '$bid, "zzzZZZ", $action, $target, $selector,' not in cna:
    fail("CNA does not pack the canonical selector directly")

manifest = json.loads(
    (ROOT / "integrations" / "sliver" / "extension.json").read_text()
)
sliver_names = {command["command_name"] for command in manifest["commands"]}
expected_sliver_names = {
    f"rex-{selector}" for selector in EXPECTED.values()
}
if sliver_names != expected_sliver_names:
    fail(
        f"Sliver commands are {sorted(sliver_names)!r}, "
        f"expected {sorted(expected_sliver_names)!r}"
    )

runtime = (ROOT / "src" / "common" / "rex.c").read_text()
if "output.technique = technique->selector;" not in runtime:
    fail("runtime does not emit the canonical descriptor selector")

legacy_transcript = re.compile(
    r"technique=(WMIProvider|WMIService|WMIExec|WSManExec|TaskSchedExec|"
    r"DCOMIShellDispatch2|SCMChangeService|SCMCreateService)\b"
)
pages = [ROOT / "README.md", *sorted((ROOT / "docs").rglob("*.md"))]
pages.append(ROOT / "docs" / "demo" / "demo.toml")
pages.extend(sorted((ROOT / "docs" / "demo").glob("*.svg")))
for page in pages:
    if legacy_transcript.search(page.read_text()):
        fail(f"legacy transcript selector remains in {page.relative_to(ROOT)}")

print("checked 8 native, Cobalt Strike, Sliver, and transcript technique names")
