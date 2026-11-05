# Using an existing service

After changing what an existing service loads, starting the service starts the
process that reaches the payload. A successful start request is not proof of
payload execution; confirm that the expected payload callback ran.

## Verified service pairings

| Service | Starting state | What changes | Start action | Execution proof |
| --- | --- | --- | --- | --- |
| `NetTcpPortSharing` | Stopped and disabled; runs as `NT AUTHORITY\LocalService` | AppDomain-manager settings and assembly for `SMSvcHost.exe` | Change to demand start, then start | Manager ran inside `SMSvcHost.exe` |
| `tzautoupdate` | Stopped and disabled; runs as `NT AUTHORITY\LocalService` | `Parameters\ServiceDll` | Change to demand start, then start | Replacement `ServiceMain` ran inside the existing service host |

Do not treat a similarly named service as interchangeable. Confirm the service
name, executable path, account, start type, state, and relevant load setting.

## Workflow

1. Enumerate services and inspect the exact service. The
   [`sc_enum`, `sc_qc`, and `sc_query`](https://github.com/trustedsec/CS-Situational-Awareness-BOF)
   BOFs can supply this information.
2. Save the original configuration, load setting, start type, and state.
3. Place the compatible payload and apply the documented load change.
4. If the service is disabled, change it to demand start. A start request does
   not change the start type.
5. Start it with a tool such as
   [`sc_start`](https://github.com/trustedsec/cs-remote-ops-bof).
6. Confirm that the expected callback ran in the expected service process.
7. Restore the load setting, start type, and stopped state, then remove the
   payload.

Use the [AppDomain-manager](appdomain-manager.md) or
[ServiceDll](service-dll.md) page for the exact callback and cleanup
requirements.

## What defenders can observe

Useful signals include a disabled service being changed to demand start,
unexpected service starts, changes below the service registry key, new
assemblies or DLLs, and edits to a service executable's `.config` file.
Starting the service runs the payload as the service account, not as the
operator's remote-authentication identity.
