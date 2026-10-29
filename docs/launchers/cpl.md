# Control Panel modules

`control.exe` requires a real Control Panel module, not an arbitrary DLL renamed
to `.cpl`. The native module must export `CPlApplet` with the Control Panel
callback ABI.

```c
#include <windows.h>
#include <cpl.h>

__declspec(dllexport)
LONG CALLBACK CPlApplet(HWND window, UINT message, LPARAM first, LPARAM second) {
    switch (message) {
    case CPL_INIT:
        return TRUE;
    case CPL_GETCOUNT:
        return 0;
    default:
        return 0;
    }
}
```

At minimum, handle `CPL_INIT` and `CPL_GETCOUNT`. A module that advertises
applets must also implement the information and invocation messages those
applets require. Confirm the undecorated `CPlApplet` export and match the module
architecture to the selected `control.exe`.

```text
rex wsman-exec --target 10.0.0.15 --payload-type cpl \
  --remote-path C:\ProgramData\module.cpl --launcher control.exe
```

`rex preflight` does not inspect `CPlApplet` or the remote file.

## Cleanup

Wait for `control.exe` to release the module before deleting it. Remove any
files, processes, or registry state created by its callbacks.

## Detection

Relevant telemetry includes Control Panel process creation, module loading,
child processes, and callback activity.

Reference: [Control Panel applications](https://learn.microsoft.com/en-us/windows/win32/shell/control-panel-applications).
