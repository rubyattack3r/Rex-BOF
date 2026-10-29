# task-sched-exec

## How it works

Rex uses the Task Scheduler COM client, which connects to the remote
`ITaskSchedulerService` RPC interface. It registers a SYSTEM task in the root
folder and starts it. Cobalt Strike can supply the program through a
[launcher contract](../launchers/README.md).

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant D as Task Scheduler COM client
    participant T as Task Scheduler
    participant P as Target process
    B->>D: Connect to the remote task service
    D->>T: Call ITaskSchedulerService over RPC
    T-->>B: Confirm task name is unused
    B->>T: Register SYSTEM task
    B->>T: Run task
    T->>P: Launch program and arguments
    B-->>B: Emit task recovery context
```

## Requirements

- Local administrator on the target
- DCOM via TCP 135 and negotiated dynamic RPC ports
- Referenced files already present on the target

## Cleanup

Delete the reported task. Delete the payload after the task and its process
release it. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes task registration, task execution, RPC activity,
and process creation.

