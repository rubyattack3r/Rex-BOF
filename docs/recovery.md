# Cleanup

Rex closes its local handles, interfaces, and sessions before returning. It
does not reverse remote changes or remove payload files. Save the transcript
when it contains recovery values.

| Technique | Emitted recovery context | Operator action |
|---|---|---|
| `wmi-provider` | Provider name, CLSID, and DLL path | Unload the provider, remove its instance and registration keys, then delete the DLL. |
| `wmi-service`, `scm-change-service` | Service name, original and applied paths, start type, and original stopped state | Stop the service, verify its current state, and restore the recorded configuration. |
| `scm-create-service` | Created service name and binary path | Stop and delete the service, then verify its absence. |
| `task-sched-exec` | Registered task name and executable | Delete the recorded task. |
| `wmi-exec`, `wsman-exec`, `dcom-ishell-dispatch2` | None | Clean up changes made by the command or payload. |

Before cleanup, compare the recorded identifiers and original values with the
current target state. Avoid wildcard deletion. A failed execution may still
contain recovery values for an earlier change.

Clean up files, child processes, registry entries, COM state, services, and
accounts created by the payload. Wait for a launcher or host process to release
a module before deleting it.

Recovery records are informational; Rex does not execute them.

