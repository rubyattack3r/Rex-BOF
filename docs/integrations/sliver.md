# Sliver integration

The Sliver bundle exposes each Rex primitive as a separate command through one
multi-command `extension.json`. The manifest selects `amd64` or `386` objects
for the active Windows target and calls the integration-specific entry point
that matches the technique's argument count.

The manifest targets the Sliver v1.7.9 schema, uses built-in `reflektor`, and
declares `coff-loader` as a fallback. See [tested platforms](../platforms.md)
for client, parser, and implant validation.

## Install

Extract `Rex-BOF-Sliver-<version>.zip`, then install its directory from the
Sliver client:

```text
extensions install /path/to/Rex-BOF-Sliver-<version>
```

Extensions are local to each Sliver client. On a target without built-in
Reflektor support, install the fallback first:

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

The Sliver manifest exposes primitive command, program, and argument fields.
It does not reproduce the Cobalt Strike launcher registry. Compose launcher
commands directly using the [launcher contracts](../launchers/index.md).

## Runtime contract

The Sliver entry points adapt the manifest arguments and then use the same
native runtime as Cobalt Strike.

Keep `extension.json` and all objects from the same bundle. The manifest and
objects are one versioned contract. Each command inherits the active Sliver
implant token, operates on one target, and emits the same typed transcript and
recovery records documented in [output](../output.md) and
[recovery](../recovery.md).
