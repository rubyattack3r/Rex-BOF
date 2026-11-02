# COM-handler task execution

Some scheduled tasks run a COM handler instead of an executable. The task
stores a CLSID, and Task Scheduler looks up that CLSID's 64-bit
`InProcServer32` value to find the handler DLL. Redirecting that value makes
the same task load a different DLL and call its task-handler interface.

```text
existing task starts
        -> Task Scheduler reads the task's CLSID
        -> COM reads the CLSID's 64-bit InProcServer32 value
        -> controlled DLL loads
        -> ITaskHandler::Start runs
```

The task definition, task name, principal, and handler CLSID remain unchanged.

## Verified target

| Field | Tested value |
|---|---|
| Task | `\Microsoft\Windows\WlanSvc\MoProfileManagement` |
| Principal | `SYSTEM` |
| Handler CLSID | `{085EDA12-CF4A-4944-8222-8ADCADE137CB}` |
| Original COM server | `C:\Windows\System32\WlanMediaManager.dll` |
| Changed value | 64-bit `InProcServer32` for the existing CLSID |
| Start action | Run the existing task |
| Execution proof | Controlled `ITaskHandler::Start` ran and the task returned `0` |

## Payload requirements

The replacement must be an x64 in-process COM DLL that implements the task
handler interfaces, including `ITaskHandler`. It must report task status in the
way Task Scheduler expects and be readable by `SYSTEM`. Register the path in
the same 64-bit registry view used by the task host.

## Workflow

1. Confirm the full task path, principal, handler CLSID, and current state.
2. Save the original 64-bit `InProcServer32` value and its registry security.
3. Place the compatible handler DLL at an absolute path.
4. Point the existing CLSID's `InProcServer32` value to that DLL.
5. Run the existing task.
6. Confirm that `ITaskHandler::Start` ran and check the task result.
7. Restore the original COM server registration before removing the DLL.

Only this exact task and CLSID pairing was reproduced. Changing a different
COM registration is a separate test, even if the task action looks similar.

## Detection and references

Watch for changes to established COM-server paths, DLL loads from unusual
locations in Task Scheduler processes, and unexpected runs of the task.

Microsoft documents
[`TASK_ACTION_COM_HANDLER`](https://learn.microsoft.com/windows/win32/api/taskschd/ne-taskschd-task_action_type)
and the
[`ITaskHandler` interface](https://learn.microsoft.com/windows/win32/api/taskschd/nn-taskschd-itaskhandler).
