# Sliver integration

The Sliver bundle exposes each Rex primitive as a separate command through one
multi-command `extension.json`. The manifest selects `amd64` or `386` objects
for the active Windows target and calls the integration-specific entry point
that matches the technique's argument count.

The manifest targets the Sliver v1.7.9 extension schema. It selects the built-in
`reflektor` BOF executor and declares `coff-loader` as a fallback for targets
without the built-in BOF capability. The schema and packaged artifacts are
validated during `make verify` and `make release`. A clean bundle has also been
installed and command-registered with a Sliver v1.7.8 client, and Sliver v1.7.9
code has validated artifact selection, typed argument packing, executor
selection, and Reflektor request construction for every command and
architecture. Its Reflektor COFF parser has also accepted all 16 objects and
resolved their exact entry points. On 2026-10-29, official Sliver v1.7.9 x86
and x64 Windows implants then executed the packaged `rex_wmiexec.<arch>.o` object
through built-in Reflektor on Windows Server 2025 build 26100. Both
architectures passed read-only preflight without creating a marker, then
dispatched a process whose architecture-specific marker was independently
observed. The declared `coff-loader` fallback has not been live-tested.

## Install

Extract `Rex-BOF-Sliver-<version>.zip`, then install its directory from the
Sliver client:

```text
extensions install /path/to/Rex-BOF-Sliver-<version>
```

Extensions are installed in the local Sliver client, so each operator installs
the bundle separately. On a target without built-in Reflektor BOF support,
install the declared fallback first:

```text
armory install coff-loader
```

Use `extensions load <directory>` instead when the commands should exist only
for the current client process.

## Commands

| Sliver command | Required technique values | Optional technique values |
|---|---|---|
| `rex-wmi-provider` | `--target`, `--provider-name`, `--provider-clsid`, `--remote-path` | `--action` |
| `rex-wmi-service` | `--target`, `--service-name`, `--remote-path` | `--action` |
| `rex-wmi-exec` | `--target`, `--command` | `--action` |
| `rex-wsman-exec` | `--target`, `--program` | `--arguments`, `--transport`, `--action` |
| `rex-task-sched-exec` | `--target`, `--task-name`, `--program` | `--arguments`, `--action` |
| `rex-dcom-ishell-dispatch2` | `--target`, `--program` | `--arguments`, `--action` |
| `rex-scm-change-service` | `--target`, `--service-name`, `--remote-path` | `--action` |
| `rex-scm-create-service` | `--target`, `--service-name`, `--remote-path` | `--action` |

`--action` defaults to `execute`; set it to `preflight` to run the registered
read-only prerequisite callback and skip mutation.

```text
rex-wmi-exec --action preflight --target 10.0.0.15 --command "whoami.exe /all"
rex-wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
rex-wsman-exec --target 10.0.0.15 --program cmd.exe --arguments "/c whoami"
```

The kebab-case names above are the only public Sliver commands; legacy aliases
are not registered. The Sliver manifest exposes the primitive program,
argument, and complete command fields directly. Cobalt Strike launcher
selectors are CNA conveniences and are not reproduced in the Sliver manifest.
Compose the final program and arguments explicitly, following the
[launcher contracts](../launchers/index.md) when invoking a target-resident
payload.

## Runtime contract

The Sliver entry points only adapt the manifest argument count. They derive the
stable selector from the linked technique object, fill unused internal values
with empty strings, and enter the same decode, preflight, execution, result,
recovery, and release path used by Cobalt Strike.

Keep `extension.json` and all objects from the same bundle. The manifest and
objects are one versioned contract. Each command inherits the active Sliver
implant token, operates on one target, and emits the same typed transcript and
recovery records documented in [output](../output.md) and
[recovery](../recovery.md).
