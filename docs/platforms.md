# Tested environments

Support claims are tied to named test environments. Add an environment record
when the OS release, build, architecture, integration, or loader changes; do
not add another OS-specific column to every table.

## Environment registry

| Environment ID | Operating system | Host architecture | Integration and runtime | Last validated |
|---|---|---|---|---|
| `ws2025-26100-cs` | Windows Server 2025 Standard Evaluation, build 26100 | x64 | Cobalt Strike Beacon; x64 and x86 BOFs | 2026-10-29 |
| `ws2025-26100-sliver179-x64` | Windows Server 2025 Standard Evaluation, build 26100 | x64 | Sliver v1.7.9 x64 implant; built-in Reflektor | 2026-10-30 |
| `ws2025-26100-sliver179-x86` | Windows Server 2025 Standard Evaluation, build 26100 | x64 | Sliver v1.7.9 x86 implant; built-in Reflektor | 2026-10-30 |
| `ws2025-26100-local` | Windows Server 2025 Standard Evaluation, build 26100 | x64 | Target-local launcher contract fixtures | 2026-10-29 |
| `ws2025-26100-standalone` | Windows Server 2025 Standard Evaluation, build 26100 | x64 | Standalone x64 and x86 executables | 2026-10-30 |

Other Windows releases and builds have not been tested.

The standalone x86 and x64 executables are also build-verified as Windows
console PEs.

## Technique validation

| Technique | Environment | BOF architecture | Status | Observed evidence |
|---|---|---|---|---|
| [`wmi-provider`](techniques/wmiprovider.md) | `ws2025-26100-cs` | x64, x86 | Passed | Provider loaded with an architecture-matched DLL. |
| [`wmi-instance-provider`](techniques/wmiinstanceprovider.md) | `ws2025-26100-sliver179-x64` | x64 | Passed | Preflight left no WMI or registry state; execute loaded the architecture-matched DLL, and a class enumeration reached its instance-provider callback. |
| [`wmi-instance-provider`](techniques/wmiinstanceprovider.md) | `ws2025-26100-sliver179-x86` | x86 | Passed | Preflight left no WMI or registry state; execute loaded the architecture-matched DLL through the 32-bit registry view, and a class enumeration reached its callback. |
| [`wmi-method-provider`](techniques/wmimethodprovider.md) | `ws2025-26100-sliver179-x64` | x64 | Passed | Preflight left no WMI or registry state; execute loaded the architecture-matched DLL, and a class method call reached its method-provider callback. |
| [`wmi-method-provider`](techniques/wmimethodprovider.md) | `ws2025-26100-sliver179-x86` | x86 | Passed | Preflight left no WMI or registry state; execute loaded the architecture-matched DLL through the 32-bit registry view, and a class method call reached its callback. |
| [`wmi-event-subscription`](techniques/wmieventsubscription.md) | `ws2025-26100-sliver179-x64` | x64 | Passed | Preflight left no subscription; execute created the filter, consumer, and binding, the event wrote its exact marker, and all three objects were removed. |
| [`wmi-service`](techniques/wmiservice.md) | `ws2025-26100-cs` | x64, x86 | Passed | Service start requested and original configuration recorded. |
| [`wmi-exec`](techniques/wmiexec.md) | `ws2025-26100-cs` | x64, x86 | Passed | Process creation returned a PID and the benign marker appeared. |
| [`wmi-exec`](techniques/wmiexec.md) | `ws2025-26100-sliver179-x64` | x64 | Passed | Preflight made no marker; execute returned a PID and wrote the marker. |
| [`wmi-exec`](techniques/wmiexec.md) | `ws2025-26100-sliver179-x86` | x86 | Passed | Preflight made no marker; execute returned a PID and wrote the marker. |
| [`wmi-exec`](techniques/wmiexec.md) | `ws2025-26100-standalone` | x64, x86 | Passed | UPN and down-level make-token preflight succeeded; both architectures returned a PID and wrote exact markers. Invalid credentials failed during remote preflight with access denied. |
| [`wsman-exec`](techniques/wsmanexec.md) | `ws2025-26100-cs` | x64, x86 | Passed | Command returned status 0 and the benign marker appeared. |
| [`task-sched-exec`](techniques/taskschedexec.md) | `ws2025-26100-cs` | x64, x86 | Passed | Task returned result `0` and deletion data was recorded. |
| [`dcom-ishell-dispatch2`](techniques/dcomishelldispatch2.md) | `ws2025-26100-cs` | x64, x86 | Passed | Program started through an interactive Explorer session. |
| [`dcom-mmc20`](techniques/dcommmc20.md) candidate | `ws2025-26100-sliver179-x64` | x64 | Failed; withheld | Native and PowerShell controls succeeded. Reflektor reached `IDispatch::Invoke`, then blocked without creating a marker. A cancellable worker returned `0x800705b4` after five seconds and kept the implant alive, but left `mmc.exe` on the target. |
| [`dcom-shell-browser-window`](techniques/dcomshellbrowserwindow.md) candidate | `ws2025-26100-sliver179-x64` | x64 | Failed; withheld | An Administrator RDP session and Explorer were active. BOF preflight and direct PowerShell activation under that interactive token returned `0x80004005`; no marker appeared. |
| [`scm-change-service`](techniques/scmchangeservice.md) | `ws2025-26100-cs` | x64, x86 | Passed | Service start requested and original configuration recorded. |
| [`scm-create-service`](techniques/scmcreateservice.md) | `ws2025-26100-cs` | x64, x86 | Passed | Service start requested and deletion data was recorded. |
| [`scm-command`](techniques/scmcommand.md) | `ws2025-26100-sliver179-x64` | x64 | Passed | Preflight confirmed the name was unused; execute created and started the command-backed service, wrote the exact marker, and reported the service name for deletion. |

## Launcher validation

Launcher evidence uses the same environment registry. A program being present
does not count as a validated launcher contract.

| Launcher or payload contract | Environment | Technique or harness | Status | Observed evidence |
|---|---|---|---|---|
| Built-in DLL, CPL, managed, and custom launchers | `ws2025-26100-cs` | x64 `wmi-exec` | Passed | Each format-valid fixture ran its expected callback. |
| `rundll32.exe` DLL contract | `ws2025-26100-cs` | `wsman-exec`, `task-sched-exec`, `dcom-ishell-dispatch2` | Passed | Each technique started the same benign fixture. |
| `ps1` | `ws2025-26100-local` | `powershell.exe -File` | Passed | Process exited 0 and wrote its exact marker. |
| `bat`, `cmd` | `ws2025-26100-local` | `cmd.exe /d /s /c call` | Passed | Each process exited 0 and wrote its exact marker. |
| `vbs`, `js`, `wsf` | `ws2025-26100-local` | `cscript.exe //B //NoLogo` | Passed | Each process exited 0 and wrote its exact marker. |
| `hta` | `ws2025-26100-local` | `mshta.exe` | Passed | The fixture exited 0 and wrote its exact marker. |
| MSBuild project | `ws2025-26100-local` | .NET Framework 4.8 `MSBuild.exe` | Passed | A format-valid project exited 0 and wrote its exact marker. |
| MSI, CHM, MOF, XBAP | `ws2025-26100-local` | Presence check only | Not tested | The associated programs were present, but no payload contract ran. |

## Integration boundaries

- Sliver v1.7.8 installed a clean eight-command v0.2.0 bundle and registered
  both Windows architectures.
- Sliver v1.7.9 parsed that manifest, resolved all 16 original objects and
  entry points, and built Reflektor requests.
- The release bundle registers twelve commands and resolves 24 objects. Technique
  rows above record live execution separately from registration.
- The standalone x64 no-echo prompt was exercised from an interactive Windows
  console. Inline credentials were exercised with both standalone
  architectures; valid credentials succeeded, invalid credentials failed, and
  the temporary executables and marker files were removed after validation.
- COFFLoader v1.0.16 registered under Sliver v1.7.9, but `LoadAndRun` returned
  `ERROR_BAD_ENVIRONMENT` or `ERROR_INVALID_PARAMETER` before the BOF ran. The
  release manifest therefore requires built-in Reflektor and declares no
  fallback.

Windows CI separately tests transcript encoding for whitespace, delimiters,
control bytes, paths, Japanese text, sentinels, null values, and invalid
UTF-16. `dcom-ishell-dispatch2` requires a usable interactive shell.
The withheld `dcom-shell-browser-window` candidate additionally requires the
target platform to allow remote activation of that class; an active Explorer
process did not satisfy that condition on Server 2025 build 26100.
The three WMI provider techniques need a provider DLL with the same
architecture as the BOF and requested CLSID. Instance and method roles also
need a compatible managed-class schema in `root\default`.
