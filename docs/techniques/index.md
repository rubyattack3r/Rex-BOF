# Technique reference

Each selector has separate x86 and x64 objects. The integration selects the
matching architecture and uses the active implant token. Each command targets
one host.

| Selector | Required options | Remote API | Success evidence | Recovery context |
|---|---|---|---|---|
| [`wmi-provider`](wmiprovider.md) | `--provider-name`, `--provider-clsid`, `--remote-path` | WMI and remote registry | Provider loaded | Provider name, CLSID, and DLL path |
| [`wmi-service`](wmiservice.md) | `--service-name`, `--remote-path` | WMI `Win32_Service` | Service start requested | Original and applied service configuration |
| [`wmi-exec`](wmiexec.md) | `--command` or launcher options | WMI `Win32_Process.Create` | Process created with PID | None |
| [`wsman-exec`](wsmanexec.md) | `--program`, optional `--arguments` and `--transport`, or launcher options | WSMan/WinRM | Command exit status | None |
| [`task-sched-exec`](taskschedexec.md) | `--task-name`, `--program`, optional `--arguments`, or launcher options | Task Scheduler DCOM | Task started | Registered task name and executable |
| [`dcom-ishell-dispatch2`](dcomishelldispatch2.md) | `--program`, optional `--arguments`, or launcher options | DCOM Shell automation | Program started | None |
| [`scm-change-service`](scmchangeservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start requested | Original and applied service configuration |
| [`scm-create-service`](scmcreateservice.md) | `--service-name`, `--remote-path` | SCM RPC | Service start requested | Created service name and binary path |

Use `rex preflight <technique>` with the same options to check requirements
without executing the technique. See [preflight](../preflight.md) for the checks.

Most techniques report `dispatched` after the remote API accepts the request.
`wsman-exec` waits for the command and reports `completed` when it exits with
status 0. See [transcript output](../output.md).

