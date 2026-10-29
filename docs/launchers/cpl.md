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
rex WSManExec --target 10.0.0.15 --payload-type cpl \
  --remote-path C:\ProgramData\module.cpl --launcher control.exe
```

`rex preflight` validates the remote-execution technique only. It does not
inspect `CPlApplet`, confirm the file exists, or prove Control Panel will load
the module.

## Operator recovery

Wait for `control.exe` to release the module before removing it. Callback-created files, processes, registry state, and other effects require a payload-specific recovery procedure.

## Detection considerations

Control Panel process creation, module loading, child processes, and callback
effects are observable. No CPL invocation is undetectable.

Reference: [Control Panel applications](https://learn.microsoft.com/en-us/windows/win32/shell/control-panel-applications).
