# Cobalt Strike integration

`rex.cna` packs named options for the selected technique object. Keep the script
and objects from the same release. Each command targets one host and uses the
active Beacon token unless command-scoped credentials are supplied.

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
| `wmi-instance-provider` | `--provider-name`, `--provider-clsid`, `--remote-path` |
| `wmi-method-provider` | `--provider-name`, `--provider-clsid`, `--remote-path` |
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

Rex uses the active Beacon token by default. To use network credentials for one
invocation, add both options:

```text
rex preflight wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token DOMAIN\operator --password <password>

rex preflight wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token operator@example.com --password <password>
```

The first command uses down-level syntax; the second uses UPN syntax. Rex
obtains a `LOGON32_LOGON_NEW_CREDENTIALS` token while preserving the task
thread's current local identity. A joined worker impersonates it, runs
preflight and execution, and reverts; the task thread closes the token before
writing the result. Rex does not replace Beacon's task-thread token. Windows
may defer validation of new-credentials passwords until the first remote
access.

Cobalt Strike has no secure password prompt for a BOF command. The password is
part of operator input and tasking and may remain in console, team-server, or
audit history even though Rex clears its local packed buffer and never writes
the credential to transcript or recovery output. Prefer the active token when
credential retention is unacceptable.

The effective token needs the privileges required by the technique. The remote
API must be reachable and allowed by target policy. Make-token controls remote
authentication; it does not necessarily become the identity of the launched
process, provider, task, or service. See
[identity and command-scoped credentials](../identity.md) for the
per-technique behavior and Server 2025 results.

## Paths and text

Executable and DLL paths must already exist on the target. Rex does not create
directories, transfer files, or delete payloads.

The CNA packs the canonical kebab-case selector and target as narrow ASCII and
technique values as UTF-16. Legacy command aliases are not accepted. Unicode
command text, Windows paths, provider names, and subscription names do not
depend on the target's active code page. Rex rejects malformed UTF-16 and
control characters in provider and subscription names and escapes those names
when it builds WMI object paths. Targets accept hostnames or address literals
made from letters, digits, `.`, `-`, `_`, and `:`. Technique values are limited
to 8,191 UTF-16 code units; provider names are limited to 255 code units.

## Launchers

`wmi-exec`, `wsman-exec`, `task-sched-exec`, `dcom-ishell-dispatch2`, and
`scm-command` support built-in and custom launchers. Launcher mode requires `--payload-type`,
an absolute `--remote-path`, and `--launcher`.

```text
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\loader.dll \
  --launcher rundll32.exe --entry-point Run
```

Service techniques require a service executable. The three WMI provider
techniques load their DLL directly and do not accept launcher options. See the
[target-side input contracts](../contracts/README.md) for direct inputs and the
[launcher reference](../launchers/README.md) for launcher-resolved payloads.

## Technique-specific boundaries

`wsman-exec` defaults to `http:5985`. Accepted `--transport` values are `http`,
`https`, `http:<port>`, and `https:<port>`; HTTPS defaults to port 5986.

The three WMI provider techniques require a DLL matching the BOF architecture.
An x64 object uses the 64-bit COM registry view; an x86 object uses the 32-bit
view. Instance and method providers also require a compatible managed-class
schema in `root\default`.
`dcom-ishell-dispatch2` requires an interactive Explorer shell.
Existing-service techniques require a stopped service.

A BOF occupies its Beacon thread until the remote API call returns. Use a
dedicated session when DCOM calls may take a long time.

