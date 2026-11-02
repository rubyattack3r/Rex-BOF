# ServiceDll service execution

The `tzautoupdate` service is hosted by `svchost.exe`. Its
`Parameters\ServiceDll` registry value selects the DLL that implements the
service. Redirecting that value keeps the existing service name, account, and
host executable, but makes the host load a different DLL and call its service
entry point.

```text
existing service starts
        -> existing svchost.exe starts
        -> ServiceDll value selects the DLL
        -> controlled DLL loads
        -> ServiceMain runs
```

## How this differs from changing ImagePath

| Change | Result |
|---|---|
| Service `ImagePath` | SCM starts a different service executable |
| `Parameters\ServiceDll` | The existing `svchost.exe` host loads a different service DLL |

Rex [`scm-change-service`](../techniques/scmchangeservice.md) implements the
first path. This page documents the second.

## Verified target

| Field | Tested value |
|---|---|
| Service | `tzautoupdate` |
| Starting state | Stopped and disabled |
| Service account | `NT AUTHORITY\LocalService` |
| Host | Existing `svchost.exe` service host |
| Changed value | The service's existing `Parameters\ServiceDll` value |
| Start action | Change to demand start, then start the service |
| Execution proof | Replacement `ServiceMain` ran inside the existing service host |

## Payload requirements

The replacement must be an x64 service DLL that exports the configured service
entry point, normally `ServiceMain`. The DLL and all dependencies must be
readable by `NT AUTHORITY\LocalService` and must behave as a Windows service
DLL.

## Workflow

1. Confirm the service name, host, account, state, and start type.
2. Save the original `ServiceDll`, service entry-point setting, registry
   security, start type, and state.
3. Place the compatible service DLL at an absolute path.
4. Point the existing `ServiceDll` value to that path.
5. Change the service from disabled to demand start.
6. Start the service and confirm that `ServiceMain` ran inside its host.
7. Restore `ServiceDll`, the start type, and the stopped state before removing
   the DLL.

Only this exact `tzautoupdate` pairing was reproduced. A service that runs its
own executable, or a hosted service that expects a different entry point,
requires a separate test.

## Detection and references

Watch for changes to `ServiceDll`, a disabled service changing to demand start,
unexpected service starts, and DLL loads from unusual paths in `svchost.exe`.

Microsoft documents the
[`ServiceMain` entry point](https://learn.microsoft.com/windows/win32/services/servicemain-function)
and
[`Service Control Manager`](https://learn.microsoft.com/windows/win32/services/service-control-manager).
