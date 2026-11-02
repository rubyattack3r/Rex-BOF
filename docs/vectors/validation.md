# Tested vectors

Every row below means that a controlled callback ran in the expected process.
A task or service start request alone was not counted as execution.

## Test environment

| Field | Value |
|---|---|
| Operating system | Windows Server 2025 Standard Evaluation |
| Build | 26100, Desktop Experience |
| Architecture | x64 |
| Test date | 2026-11-02 |

## Results

| Vector | Existing target | Starting state | What changed | How it was started | Execution proof | Cleanup checked |
|---|---|---|---|---|---|---|
| AppDomain manager | `\Microsoft\Windows\Server Initial Configuration Task` | Disabled | `srvinitconfig.exe.config` and manager assembly | Task enabled and run | Manager ran inside `srvinitconfig.exe` as `SYSTEM` | Yes |
| AppDomain manager | `NetTcpPortSharing` | Stopped and disabled | `SMSvcHost.exe.config` and manager assembly | Changed to demand start and started | Manager ran inside `SMSvcHost.exe` as `NT AUTHORITY\LocalService` | Yes |
| COM task handler | `\Microsoft\Windows\WlanSvc\MoProfileManagement` | Ready | Existing CLSID's 64-bit `InProcServer32` | Existing task run | `ITaskHandler::Start` ran; task result `0` | Yes |
| ServiceDll | `tzautoupdate` | Stopped and disabled | Existing `Parameters\ServiceDll` | Changed to demand start and started | Replacement `ServiceMain` ran inside the existing service host as `NT AUTHORITY\LocalService` | Yes |

## What these results establish

- The listed Windows object was present with the recorded action or host.
- The documented load setting could be redirected with administrative access.
- Starting that exact object reached the required payload callback.
- The original setting and task or service state could be restored.

The results do not establish that a different Windows build, installation
type, language, architecture, or similarly named object behaves the same way.
Add a new platform result only after callback execution and cleanup are both
observed on that platform.
