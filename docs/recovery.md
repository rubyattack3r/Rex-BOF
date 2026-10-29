# Operator recovery

`rex` releases process-local handles, COM interfaces, and transport sessions before returning, but it does not automatically reverse remote assessment state. The BOF also does not stage or remove payload files. Save the transcript when recovery context matters to the engagement.

| Technique | Emitted recovery context | Operator action |
|---|---|---|
| `WMIProvider` | Provider name, CLSID, and DLL path | Unload the provider, remove its instance and registration keys, then remove the separately delivered DLL after release. |
| `WMIService`, `SCMChangeService` | Service name, original and applied binary paths, original start type, and preflight-proven stopped state | Stop the service if safe, verify current state, and restore the recorded configuration. |
| `SCMCreateService` | Created service name and binary path | Stop and delete the exact service, then verify its absence. |
| `TaskSchedExec` | Registered task name and executable | Delete the exact registered task after collecting the required evidence. |
| `WMIExec`, `WSManExec`, `DCOMIShellDispatch2` | No generic persistent configuration record | Review process or command-defined effects separately. |

Before recovery, compare every identifier and original value with current target state. Never delete similarly named services, tasks, provider registrations, files, or registry keys by wildcard. An execution failure can still carry recovery context when mutation occurred before the failure.

Launcher and payload callbacks can create files, child processes, registry entries, COM state, services, accounts, or other effects that the BOF cannot infer. Registration, installer, CPL, injection, and custom-launcher effects require payload-specific recovery. Wait until the launcher or hosting process releases a module before attempting to remove its separately delivered file.

The recovery record is operator-visible context, not an executable rollback journal. Losing the transcript loses that context.

