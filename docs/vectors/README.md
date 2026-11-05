# Verified execution vectors

Rex provides remote-execution techniques. These pages cover a related
workflow: reusing a scheduled task or service that Windows already provides.
Rex does not add commands for these workflows. Use existing discovery and
activation tooling after confirming that the target matches the tested object.

Every vector listed here reached a controlled callback. A successful task or
service start is not, by itself, proof that the payload ran.

Here, **existing** means the task or service is already registered on Windows.
It does not mean that an already-running process can be taken over. Each
verified path needs a new task run or service start after the load setting is
changed.

## Operator workflow

1. Confirm the exact task or service, action, identity, and current state.
2. Record every value that will be changed.
3. Place a payload that implements the required callback.
4. Change the documented load setting.
5. Enable and start the existing object when required.
6. Confirm that the payload callback ran in the expected process.
7. Restore the original values and state before removing the payload.

These vectors require permission to change the relevant task, service, file,
or registry value. They do not provide a privilege escalation by themselves.

## Verified vectors

| Existing Windows object | What changes | How it is started | Execution identity | Proof obtained |
| --- | --- | --- | --- | --- |
| `\Microsoft\Windows\Server Initial Configuration Task` | AppDomain-manager settings for `srvinitconfig.exe` | Enable and run the task | `SYSTEM` | The manager ran inside `srvinitconfig.exe` |
| `NetTcpPortSharing` | AppDomain-manager settings for `SMSvcHost.exe` | Change to demand start and start the service | `NT AUTHORITY\LocalService` | The manager ran inside `SMSvcHost.exe` |
| `\Microsoft\Windows\WlanSvc\MoProfileManagement` | The 64-bit COM server for the task's existing CLSID | Run the task | `SYSTEM` | `ITaskHandler::Start` ran and the task returned `0` |
| `tzautoupdate` | The service's `ServiceDll` value | Change to demand start and start the service | `NT AUTHORITY\LocalService` | The replacement `ServiceMain` ran inside the existing service host |

## Read the vector that matches the target

- [Existing scheduled tasks](scheduled-task.md)
- [Existing services](service.md)
- [AppDomain-manager execution](appdomain-manager.md)
- [COM-handler task execution](task-com-handler.md)
- [ServiceDll service execution](service-dll.md)
- [Tested vectors and evidence](validation.md)

## ServiceDll is not the service executable

These are two different ways to change what a service runs:

| Method | Value changed | What Windows runs |
| --- | --- | --- |
| Rex [`scm-change-service`](../techniques/scmchangeservice.md) | Service `ImagePath` | A different service executable |
| [ServiceDll redirection](service-dll.md) | `Parameters\ServiceDll` | A different DLL inside the service's existing `svchost.exe` process |

Both paths produced execution in testing. Only the second is one of the
existing-object vectors documented here.
