# rex BOF documentation

`rex` provides native x86 and x64 Beacon Object Files for authorized remote-execution assessments against Windows estates. Each technique has a separate object, while `rex.cna` provides one technique-aware command surface and selects the object matching the active Beacon architecture.

The BOFs inherit the active Beacon token and operate on one target per invocation. They perform technique preflight before mutation, but do not stage or delete payloads, accept credentials, or batch targets. No technique is undetectable; visibility depends on the target's endpoint, operating-system, identity, and network telemetry.

| Selector | Input | Privilege | Surface | Outcome and recovery |
|---|---|---|---|---|
| [WMIProvider](techniques/wmiprovider.md) | Provider DLL | Local administrator | WMI/DCOM, remote registry | Provider load completed; provider registration retained |
| [WMIService](techniques/wmiservice.md) | Service executable | Local administrator | WMI/DCOM | Service start dispatched; changed service retained |
| [WMIExec](techniques/wmiexec.md) | Command or launcher | Local administrator | WMI/DCOM | Process dispatched with PID; no generic configuration record |
| [WSManExec](techniques/wsmanexec.md) | Program or launcher | WinRM-authorized administrator | WinRM | Command completed; no generic configuration record |
| [TaskSchedExec](techniques/taskschedexec.md) | Program or launcher | Local administrator | DCOM/RPC | Task dispatched; task registration retained |
| [DCOMIShellDispatch2](techniques/dcomishelldispatch2.md) | Program or launcher | Local administrator plus active shell | DCOM/RPC | Shell execution dispatched; no generic configuration record |
| [SCMChangeService](techniques/scmchangeservice.md) | Service executable | Local administrator | SCM/RPC | Service start dispatched; changed service retained |
| [SCMCreateService](techniques/scmcreateservice.md) | Service executable | Local administrator | SCM/RPC | Service start dispatched; created service retained |

Additional references:

- [Runtime configuration](configuration.md)
- [Technique reference](techniques/index.md)
- [Read-only preflight](preflight.md)
- [Launcher contracts](launchers/index.md)
- [Transcript output](output.md)
- [Operator recovery](recovery.md)
- [Tested platforms](platforms.md)
- [Architecture and development](architecture.md)
- [Acceptable Use Policy](../ACCEPTABLE_USE_POLICY.md)

Load `integrations/cna/rex.cna` with the adjacent per-technique objects from the same build. Run `rex help`, `rex techniques`, or `rex help <technique>` for contextual usage.

