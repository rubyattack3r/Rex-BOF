# Target-side input contracts

Five stable techniques use target-side inputs directly instead of resolving a
launcher. The input must already satisfy the Windows interface that consumes
it; Rex does not transfer, inspect, or adapt it.

| Techniques | Required contract |
| --- | --- |
| [`wmi-provider`](../techniques/wmiprovider.md) | [WMI provider COM DLL](wmi-provider.md) |
| [`wmi-event-subscription`](../techniques/wmieventsubscription.md) | [WMI event-consumer command line](wmi-event-command.md) |
| [`wmi-service`](../techniques/wmiservice.md), [`scm-change-service`](../techniques/scmchangeservice.md), [`scm-create-service`](../techniques/scmcreateservice.md) | [Windows service executable](service-executable.md) |

Validate the final target-side file or command in a disposable environment.
Preflight checks the remote technique boundary, not the input ABI, file,
dependencies, query behavior, or eventual execution.

Techniques that resolve a payload through another executable use the separate
[launcher contracts](../launchers/README.md).
