# Windows service executable contract

`wmi-service`, `scm-change-service`, and `scm-create-service` pass an image
path directly to the Windows Service Control Manager. They do not use a
launcher and do not turn an ordinary console program into a service.

## Required contract

The target-side executable must:

1. Be a Windows service program supported by the target architecture. Its
   architecture does not need to match the BOF because SCM starts it in a
   separate process.
2. Call `StartServiceCtrlDispatcher` promptly after process startup and expose
   the expected `ServiceMain` entry through its service table.
3. Register a control handler, report valid service-state transitions, and
   handle stop requests needed for cleanup.
4. Resolve its dependencies and access files, registry keys, and network
   resources under the configured service identity.
5. Avoid assumptions about an interactive desktop, the operator's profile,
   mapped drives, or the current working directory.

Supply the exact SCM binary-path string. Use an absolute target-local path and
Windows quoting when the path contains spaces or includes service arguments.
Rex calls the service with no additional start arguments.

See Microsoft's [service program](https://learn.microsoft.com/en-us/windows/win32/services/service-programs)
and [`ServiceMain`](https://learn.microsoft.com/en-us/windows/win32/services/writing-a-servicemain-function)
guidance.

## Technique behavior

| Technique | Service configuration |
| --- | --- |
| `wmi-service` | Requires an existing stopped service, changes its `PathName` through `Win32_Service.Change`, preserves its existing service account and start mode, then requests a start. |
| `scm-change-service` | Requires an existing stopped service, changes its binary path, sets demand start, preserves its existing service account, then requests a start. |
| `scm-create-service` | Creates a demand-start `SERVICE_WIN32_OWN_PROCESS` service under LocalSystem, then requests a start. |

The service name supplied to Rex is the name SCM starts. A replacement service
used with an existing registration must not assume that its own build-time
name matches that registration.

## Validation

Before execution:

1. Install and start the executable as a disposable service with the same
   account, service name, binary-path string, and target architecture.
2. Confirm that startup returns through SCM without
   `ERROR_FAILED_SERVICE_CONTROLLER_CONNECT` or a start timeout.
3. Exercise stop handling and verify that the process exits and releases its
   files.
4. Record every file, registry, network, and child-process side effect.

Rex preflight does not open the executable or inspect its service table,
architecture, dependencies, path quoting, or startup behavior. A `dispatched`
result means the service start request was accepted; it does not prove that
the service completed its work.
