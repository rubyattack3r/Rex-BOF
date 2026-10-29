# Runtime configuration

`rex.cna` converts named operator options into the fixed typed envelope consumed by each technique object. The script and objects must come from the same build. One invocation operates on one target and inherits the active Beacon token.

## Common command surface

```text
rex help
rex techniques
rex launchers
rex help WMIExec
rex preflight WMIExec --target 10.0.0.15 --command "whoami.exe /all"
rex WMIExec --target 10.0.0.15 --command "whoami.exe /all"
```

Prefix a normal invocation with `preflight` to run the same registered prerequisite callback and skip execution. The technique options remain required because preflight validates the exact request that would otherwise execute.

| Selector | Technique options |
|---|---|
| `WMIProvider` | `--provider-name`, `--provider-clsid`, `--remote-path` |
| `WMIService` | `--service-name`, `--remote-path` |
| `WMIExec` | `--command` or launcher options |
| `WSManExec` | `--program`, optional `--arguments`, optional `--transport`, or launcher options |
| `TaskSchedExec` | `--task-name`, `--program`, optional `--arguments`, or launcher options |
| `DCOMIShellDispatch2` | `--program`, optional `--arguments`, or launcher options |
| `SCMChangeService` | `--service-name`, `--remote-path` |
| `SCMCreateService` | `--service-name`, `--remote-path` |

## Identity boundary

`rex` uses the active Beacon token. It does not accept a domain, username, password, ticket, or token selector, and it never creates, replaces, or reverts impersonation. Establish the intended identity before invoking `rex` and revert it explicitly when the operation is complete.

The effective token must have the privileges required by the selected technique, the remote API must be reachable, and target policy must allow the operation.

## Paths and text

Every executable or DLL path must already exist on the target. The BOFs do not stage files, create parent directories, or delete payloads. Delivery and file-artifact tracking remain separate operator actions.

The CNA packs the target and stable selector as narrow ASCII and technique values as UTF-16. Unicode command text and Windows paths, including Japanese names, do not depend on the target's active code page. Targets accept hostnames or address literals made from letters, digits, `.`, `-`, `_`, and `:`. Technique values are limited to 8,191 UTF-16 code units.

## Launchers

`WMIExec`, `WSManExec`, `TaskSchedExec`, and `DCOMIShellDispatch2` can resolve built-in or custom launchers into their existing command primitives. Launcher mode requires `--payload-type`, an absolute `--remote-path`, and `--launcher`. The referenced payload must already exist remotely.

```text
rex WMIExec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\loader.dll \
  --launcher rundll32.exe --entry-point Run
```

Service techniques require a service-compatible executable. `WMIProvider` loads its provider DLL directly and does not accept launcher options. See the [launcher reference](launchers/index.md) for exact selectors and callback contracts.

## Technique-specific boundaries

`WSManExec` defaults to `http:5985`. Accepted `--transport` values are `http`, `https`, `http:<port>`, and `https:<port>`; HTTPS without an explicit port defaults to 5986. Invalid schemes and ports fail during input validation before session creation.

`WMIProvider` must use a provider DLL matching the BOF architecture. An x64 object uses the 64-bit COM registry view; an x86 object uses the 32-bit view. `DCOMIShellDispatch2` additionally requires a usable interactive Explorer shell. Existing-service techniques require the selected service to be stopped before mutation.

Unlike a standalone CLI, a BOF occupies the Beacon thread until the remote API call returns. Use a dedicated session when a target or firewall may leave DCOM calls waiting.

