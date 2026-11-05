# Sliver integration

The Sliver bundle provides one command per technique through a single
`extension.json`. The manifest picks the `amd64` or `386` object that matches
the implant's architecture.

The manifest targets the Sliver v1.7.9 schema and requires built-in
`reflektor`. See [tested platforms](../platforms.md) for client, parser, and
implant validation.

## Install

Extract `Rex-BOF-Sliver-<version>.zip`, then install its directory from the
Sliver client:

```text
extensions install /path/to/Rex-BOF-Sliver-<version>
```

Extensions are local to each Sliver client. Use
`extensions load <directory>` instead when the commands should exist only
for the current client process.

## Commands

| Sliver command | Required technique values | Optional technique values |
| --- | --- | --- |
| `rex-wmi-provider` | `--target`, `--provider-name`, `--provider-clsid`, `--remote-path` | `--action` |
| `rex-wmi-instance-provider` | `--target`, `--provider-name`, `--provider-clsid`, `--remote-path` | `--action` |
| `rex-wmi-method-provider` | `--target`, `--provider-name`, `--provider-clsid`, `--remote-path` | `--action` |
| `rex-wmi-event-subscription` | `--target`, `--subscription-name`, `--query`, `--command` | `--action` |
| `rex-wmi-service` | `--target`, `--service-name`, `--remote-path` | `--action` |
| `rex-wmi-exec` | `--target`, `--command` | `--action` |
| `rex-wsman-exec` | `--target`, `--program` | `--arguments`, `--transport`, `--action` |
| `rex-task-sched-exec` | `--target`, `--task-name`, `--program` | `--arguments`, `--action` |
| `rex-dcom-ishell-dispatch2` | `--target`, `--program` | `--arguments`, `--action` |
| `rex-scm-change-service` | `--target`, `--service-name`, `--remote-path` | `--action` |
| `rex-scm-create-service` | `--target`, `--service-name`, `--remote-path` | `--action` |
| `rex-scm-command` | `--target`, `--service-name`, `--command` | `--action` |

`--action` defaults to `execute`. Set it to `preflight` to run only the
read-only checks and make no changes.

Always pass values as the named options shown in the table and examples.
Sliver's usage heading lists the fields as positional placeholders, but Rex
requires the `--option` form.

Every command also accepts optional `--make-token` and `--password` values.
Supply both to use `DOMAIN\user` or `user@domain` network credentials for that
invocation. Without them, Rex uses the active implant token.

<!-- markdownlint-disable MD013 -->

```text
rex-wmi-exec --action preflight --target 10.0.0.15 --command "whoami.exe /all"
rex-wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
rex-wmi-exec --target server01 --command "whoami.exe /all" --make-token 'DOMAIN\operator' --password '<password>'
rex-wmi-exec --target server01 --command "whoami.exe /all" --make-token 'operator@example.com' --password '<password>'
rex-wsman-exec --target 10.0.0.15 --program cmd.exe --arguments "/c whoami"
rex-wmi-instance-provider --target server01 --provider-name ExampleProvider --provider-clsid {00000000-0000-0000-0000-000000000000} --remote-path 'C:\ProgramData\Example\provider.dll'
```

<!-- markdownlint-enable MD013 -->

Single-quote Windows paths so Sliver's command lexer preserves each
backslash. An unquoted value such as `C:\ProgramData\Example\provider.dll`
is packed without its backslashes and fails the target-side path check or DLL
load.

Command text, Windows paths, provider names, and subscription names are passed
to Rex as UTF-16. Provider and subscription names may contain multilingual
Windows text. Rex rejects malformed UTF-16 and control characters and escapes
the names when it builds WMI object paths. Provider names are limited to 255
UTF-16 code units.

The Sliver manifest exposes primitive command, program, and argument fields.
It does not reproduce the Cobalt Strike launcher registry. Compose launcher
commands directly using the [launcher contracts](../launchers/README.md).
For provider DLLs, service executables, and WMI event commands, follow the
[target-side input contracts](../contracts/README.md).

## Runtime contract

The Sliver entry points adapt the manifest arguments and then use the same
native runtime as Cobalt Strike.

Keep `extension.json` and all objects from the same bundle. The manifest and
objects are one versioned contract. Each command uses the active Sliver implant
token by default and operates on one target. With make-token options, Rex runs
the complete lifecycle on a joined worker under a temporary
`LOGON32_LOGON_NEW_CREDENTIALS` token, then reverts and closes it before
returning. Windows may defer credential validation until remote access.

Sliver cannot prompt inside a BOF, so `--password` is required with
`--make-token`. The password may remain in client, server, or audit history;
Rex clears its local packed buffer and omits credentials from transcript and
recovery output. Results otherwise use the contracts documented in
[output](../output.md) and [recovery](../recovery.md). See
[identity and command-scoped credentials](../identity.md) for the distinction
between remote authentication and target-side execution identity.

Sliver clients without built-in Reflektor cannot load these commands. Rex does
not fall back to an untested loader.
