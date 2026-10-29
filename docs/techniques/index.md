# Technique reference

Each selector maps to separate x86 and x64 objects. `rex.cna` selects the object matching the active Beacon architecture, converts named options into the typed BOF request, and uses the active Beacon token. One invocation operates on one target.

| Selector | Required options | Remote API | Success evidence | Recovery context |
|---|---|---|---|---|
| [WMIProvider](wmiprovider.md) | `--provider-name`, `--provider-clsid`, `--remote-path` | WMI and remote registry | Provider load completed | Provider name, CLSID, and DLL path |
| [WMIService](wmiservice.md) | `--service-name`, `--remote-path` | WMI `Win32_Service` | Service start dispatched | Original and applied service configuration |
| [WMIExec](wmiexec.md) | `--command` or launcher options | WMI `Win32_Process.Create` | Process dispatched with PID | None |
| [WSManExec](wsmanexec.md) | `--program`, optional `--arguments` and `--transport`, or launcher options | WSMan/WinRM | Command completed | None |
| [TaskSchedExec](taskschedexec.md) | `--task-name`, `--program`, optional `--arguments`, or launcher options | Task Scheduler DCOM | Task run dispatched | Registered task name and executable |
| [DCOMIShellDispatch2](dcomishelldispatch2.md) | `--program`, optional `--arguments`, or launcher options | DCOM Shell automation | Shell execution dispatched | None |
| [SCMChangeService](scmchangeservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start dispatched | Original and applied service configuration |
| [SCMCreateService](scmcreateservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start dispatched | Created service name and binary path |

Use `rex preflight <technique>` with the same options to run the registered read-only checks without invoking the technique's execution callback. See the [preflight boundary](../preflight.md) for the exact checks and limitations.

`dispatched` means the remote API accepted the work; it does not claim payload completion. `WSManExec` is the exception because it waits for the remote command and reports `completed` only for a zero exit status. See [results and recovery](../output.md) for the transcript contract.

