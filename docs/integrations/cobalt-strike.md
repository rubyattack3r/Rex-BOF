# Cobalt Strike integration

`rex.cna` packs named options for the selected technique object. Keep the script
and objects from the same release. Each command targets one host and uses the
active Beacon token.

## Common command surface

```text
rex help
rex techniques
rex launchers
rex help wmi-exec
rex preflight wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
rex wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
```

Prefix a command with `preflight` to check its requirements without executing
the technique. Supply the same options as an execution command.

| Command | Technique options |
|---|---|
| `wmi-provider` | `--provider-name`, `--provider-clsid`, `--remote-path` |
| `wmi-event-subscription` | `--subscription-name`, `--query`, `--command` |
| `wmi-service` | `--service-name`, `--remote-path` |
| `wmi-exec` | `--command` or launcher options |
| `wsman-exec` | `--program`, optional `--arguments`, optional `--transport`, or launcher options |
| `task-sched-exec` | `--task-name`, `--program`, optional `--arguments`, or launcher options |
| `dcom-ishell-dispatch2` | `--program`, optional `--arguments`, or launcher options |
| `scm-change-service` | `--service-name`, `--remote-path` |
| `scm-create-service` | `--service-name`, `--remote-path` |
| `scm-command` | `--service-name`, `--command`, or launcher options |

## Authentication

Rex uses the active Beacon token and has no credential or token options. Set the
required identity before running a command and revert it afterward.

The effective token needs the privileges required by the technique. The remote
API must be reachable and allowed by target policy.

## Paths and text

Executable and DLL paths must already exist on the target. Rex does not create
directories, transfer files, or delete payloads.

The CNA packs the canonical kebab-case selector and target as narrow ASCII and
technique values as UTF-16. Legacy command aliases are not accepted. Unicode
command text and Windows paths, including Japanese names, do not depend on the
target's active code page. Targets accept hostnames or address literals made
from letters, digits, `.`, `-`, `_`, and `:`. Technique values are limited to
8,191 UTF-16 code units.

## Launchers

`wmi-exec`, `wsman-exec`, `task-sched-exec`, `dcom-ishell-dispatch2`, and
`scm-command` support built-in and custom launchers. Launcher mode requires `--payload-type`,
an absolute `--remote-path`, and `--launcher`.

```text
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\loader.dll \
  --launcher rundll32.exe --entry-point Run
```

Service techniques require a service executable. `wmi-provider` loads its DLL
directly and does not accept launcher options. See the
[launcher reference](../launchers/README.md) for selectors and callback contracts.

## Technique-specific boundaries

`wsman-exec` defaults to `http:5985`. Accepted `--transport` values are `http`,
`https`, `http:<port>`, and `https:<port>`; HTTPS defaults to port 5986.

`wmi-provider` requires a DLL matching the BOF architecture. An x64 object uses
the 64-bit COM registry view; an x86 object uses the 32-bit view.
`dcom-ishell-dispatch2` requires an interactive Explorer shell.
Existing-service techniques require a stopped service.

A BOF occupies its Beacon thread until the remote API call returns. Use a
dedicated session when DCOM calls may take a long time.

