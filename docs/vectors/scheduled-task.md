# Using an existing scheduled task

After changing what an existing scheduled task loads, running the task starts
the process that reaches the payload. The run is only the trigger; execution
is proven when the expected payload callback runs.

## Verified task pairings

| Task | Starting state | What changes | Start action | Execution proof |
|---|---|---|---|---|
| `\Microsoft\Windows\Server Initial Configuration Task` | Disabled; runs as `SYSTEM` with highest privileges | AppDomain-manager settings and assembly for `srvinitconfig.exe` | Enable, then run the task | Manager ran inside `srvinitconfig.exe` |
| `\Microsoft\Windows\WlanSvc\MoProfileManagement` | Ready; runs as `SYSTEM` | 64-bit COM server for the task's existing handler CLSID | Run the task | `ITaskHandler::Start` ran; task result was `0` |

Do not treat a similarly named task as interchangeable. Confirm the full task
path, action, principal, and current state before making a change.

## Workflow

1. Enumerate tasks and inspect the exact task. The
   [`schtasksenum` and `schtasksquery`](https://github.com/trustedsec/CS-Situational-Awareness-BOF)
   BOFs can supply this information.
2. Save the original task definition, state, and load setting.
3. Place the compatible payload and apply the documented load change.
4. Enable the task if it is disabled. A run request does not enable it.
5. Request the task with a tool such as
   [`schtasksrun`](https://github.com/trustedsec/cs-remote-ops-bof).
6. Confirm that the expected callback ran. A successful run request means only
   that Task Scheduler accepted the request.
7. Restore the load setting and original task state, then remove the payload.

Use the [AppDomain-manager](appdomain-manager.md) or
[COM-handler](task-com-handler.md) page for the exact callback and cleanup
requirements.

## What defenders can observe

Useful signals include a task being enabled or started outside its normal
schedule, changes to a task handler's COM registration, new assemblies beside
a Windows executable, and changes to an executable's `.config` file. Preserve
the original task state and definition so recovery is exact.
