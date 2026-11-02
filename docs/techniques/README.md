# Technique reference

Each selector has separate x86 and x64 objects and is also linked into both
standalone executables. The integration selects the matching architecture.
Each command targets one host and uses the current identity unless
command-scoped make-token credentials are supplied.

Make-token changes the identity used for remote authentication. It does not
override the account chosen by WMI, WinRM, Task Scheduler, an existing service,
SCM, or an interactive shell for target-side execution. See the
[per-technique identity matrix](../identity.md).

Inputs consumed directly by WMI or SCM must satisfy the applicable
[target-side input contract](../contracts/README.md). Command-capable
techniques may instead use the Cobalt Strike [launcher contracts](../launchers/README.md).

| Selector | Required options | Remote API | Success evidence | Recovery context | Platform validation |
|---|---|---|---|---|---|
| [`wmi-provider`](wmiprovider.md) | `--provider-name`, `--provider-clsid`, `--remote-path` | WMI and remote registry | Initialization provider loaded | Provider role, name, CLSID, and DLL path | Windows; Server 2025 validated |
| [`wmi-instance-provider`](wmiinstanceprovider.md) | `--provider-name`, `--provider-clsid`, `--remote-path` | WMI instance-provider registration and remote registry | Instance provider loaded | Provider role, name, CLSID, and DLL path | Windows; Server 2025 Sliver x64 and x86 validated |
| [`wmi-method-provider`](wmimethodprovider.md) | `--provider-name`, `--provider-clsid`, `--remote-path` | WMI method-provider registration and remote registry | Method provider loaded | Provider role, name, CLSID, and DLL path | Windows; Server 2025 Sliver x64 and x86 validated |
| [`wmi-event-subscription`](wmieventsubscription.md) | `--subscription-name`, `--query`, `--command` | WMI permanent event consumer | Filter, consumer, and binding created | Subscription name | Windows; Server 2025 x64 validated |
| [`wmi-service`](wmiservice.md) | `--service-name`, `--remote-path` | WMI `Win32_Service` | Service start requested | Original and applied service configuration | Windows; Server 2025 validated |
| [`wmi-exec`](wmiexec.md) | `--command` or launcher options | WMI `Win32_Process.Create` | Process created with PID | None | Windows; Server 2025 validated |
| [`wsman-exec`](wsmanexec.md) | `--program`, optional `--arguments` and `--transport`, or launcher options | WSMan/WinRM | Command exit status | None | Windows; Server 2025 validated |
| [`task-sched-exec`](taskschedexec.md) | `--task-name`, `--program`, optional `--arguments`, or launcher options | Task Scheduler COM client and `ITaskSchedulerService` RPC | Task started | Registered task name and executable | Windows; Server 2025 validated |
| [`dcom-ishell-dispatch2`](dcomishelldispatch2.md) | `--program`, optional `--arguments`, or launcher options | DCOM `ShellWindows` automation | Program started | None | Windows; Server 2025 active-token and make-token validated |
| [`scm-change-service`](scmchangeservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start requested | Original and applied service configuration | Windows; Server 2025 validated |
| [`scm-create-service`](scmcreateservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start requested | Created service name and binary path | Windows; Server 2025 validated |
| [`scm-command`](scmcommand.md) | `--service-name`, `--command` or launcher options | SCM RPC | Service start requested | Created service name | Windows; Server 2025 x64 validated |

Use `rex preflight <technique>` with the same options to check requirements
without executing the technique. See [preflight](../preflight.md) for the checks.

Most techniques report `dispatched` after the remote API accepts the request.
`wsman-exec` waits for the command and reports `completed` when it exits with
status 0. See [transcript output](../output.md).

## Verified existing-object vectors

The following paths reuse a scheduled task or service already present on the
target. They are documented operator workflows, not Rex selectors or release
objects. Each row names the exact Windows object that reached a controlled
callback in testing; do not assume that a similarly named object will behave
the same way. See the [vector reference](../vectors/README.md) for the complete
steps and payload requirements.

| Verified vector | What must match | Find it | Start it | Execution proof | Restore |
|---|---|---|---|---|---|
| [`Server Initial Configuration Task` + AppDomain manager](../vectors/appdomain-manager.md) | Task has the verified `srvinitconfig.exe` action | [`schtasksenum`, `schtasksquery`](https://github.com/trustedsec/CS-Situational-Awareness-BOF) | Change the configuration, enable the task, and run it | Manager executes in `srvinitconfig.exe` | Restore configuration and task state; remove the assembly |
| [`NetTcpPortSharing` + AppDomain manager](../vectors/appdomain-manager.md) | Stopped and disabled service has the verified `SMSvcHost.exe` host | [`sc_enum`, `sc_qc`, `sc_query`](https://github.com/trustedsec/CS-Situational-Awareness-BOF) | Change the configuration, switch to demand start, and start it | Manager executes in `SMSvcHost.exe` | Restore configuration, stopped/disabled state, and assembly |
| [`MoProfileManagement` + COM-handler redirection](../vectors/task-com-handler.md) | Ready task has the verified CLSID and `SYSTEM` principal | [`schtasksenum`, `schtasksquery`](https://github.com/trustedsec/CS-Situational-Awareness-BOF) | Redirect 64-bit `InProcServer32` and run the task | `ITaskHandler::Start` executes; task result `0` | Restore the COM server registration and remove the DLL |
| [`tzautoupdate` + ServiceDll redirection](../vectors/service-dll.md) | Stopped and disabled service has the verified `LocalService` host | [`sc_enum`, `sc_qc`, `sc_query`](https://github.com/trustedsec/CS-Situational-Awareness-BOF) | Redirect `ServiceDll`, switch to demand start, and start it | `ServiceMain` executes in the service host | Restore `ServiceDll`, stopped/disabled state, and remove the DLL |

See [tested environments](../platforms.md) for the evidence behind each Rex
technique claim. Existing-object results are recorded separately in the
[vector test results](../vectors/validation.md).

## Withheld candidates

[`dcom-mmc20`](dcommmc20.md) and
[`dcom-shell-browser-window`](dcomshellbrowserwindow.md) are not included in
the Cobalt Strike or Sliver command surfaces. Their Server 2025 tests did not
produce successful execution evidence.

