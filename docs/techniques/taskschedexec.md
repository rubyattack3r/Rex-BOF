# task-sched-exec

## How it works

Rex connects to Task Scheduler over DCOM, registers a SYSTEM task in the root
folder, and starts it. Cobalt Strike can supply the program through a
[launcher contract](../launchers/index.md).

```mermaid
sequenceDiagram
    participant B as rex BOF
    participant D as DCOM
    participant T as Task Scheduler
    participant P as Target process
    B->>D: Activate Task Scheduler service
    D->>T: Verify task name is unused
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

