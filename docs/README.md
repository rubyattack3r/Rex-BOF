# Rex BOF documentation

Rex provides native x86 and x64 BOFs for ten Windows remote-execution
techniques. Cobalt Strike exposes them through one `rex` command; Sliver uses
ten `rex-*` commands.

Commands use the active implant token and operate on one target at a time. Rex
does not accept credentials or transfer payload files.

| Selector | Input | Privilege | Surface | Outcome and recovery |
|---|---|---|---|---|
| [`wmi-provider`](techniques/wmiprovider.md) | Provider DLL | Local administrator | WMI/DCOM, remote registry | Provider loaded; registration remains |
| [`wmi-event-subscription`](techniques/wmieventsubscription.md) | WQL query and command | Local administrator | WMI/DCOM | Permanent filter, consumer, and binding remain |
| [`wmi-service`](techniques/wmiservice.md) | Service executable | Local administrator | WMI/DCOM | Service start requested; configuration changed |
| [`wmi-exec`](techniques/wmiexec.md) | Command or launcher | Local administrator | WMI/DCOM | Process created; PID returned |
| [`wsman-exec`](techniques/wsmanexec.md) | Program or launcher | WinRM-authorized administrator | WinRM | Command exit status returned |
| [`task-sched-exec`](techniques/taskschedexec.md) | Program or launcher | Local administrator | DCOM/RPC | Task started; task remains registered |
| [`dcom-ishell-dispatch2`](techniques/dcomishelldispatch2.md) | Program or launcher | Local administrator plus active shell | DCOM/RPC | Shell launched the program |
| [`scm-change-service`](techniques/scmchangeservice.md) | Service executable | Local administrator | SCM/RPC | Service start requested; configuration changed |
| [`scm-create-service`](techniques/scmcreateservice.md) | Service executable | Local administrator | SCM/RPC | Service start requested; service remains registered |
| [`scm-command`](techniques/scmcommand.md) | Command or launcher | Local administrator | SCM/RPC | Service start requested; service remains registered |

Additional references:

- [Integration guides](integrations/README.md)
- [Technique reference](techniques/README.md)
- [Read-only preflight](preflight.md)
- [Launcher contracts](launchers/README.md)
- [Transcript output](output.md)
- [Cleanup](recovery.md)
- [Tested platforms](platforms.md)
- [Architecture and development](architecture.md)
- [Acceptable Use Policy](../ACCEPTABLE_USE_POLICY.md)

Keep the integration file and its object files together from the same release.
The [MMC20](techniques/dcommmc20.md) and
[ShellBrowserWindow](techniques/dcomshellbrowserwindow.md) candidates are
documented with their Server 2025 test results but are not release commands.
