# wmi-exec

## How it works

Rex calls WMI `Win32_Process.Create` with a complete command line. Cobalt Strike
can build that command from a [launcher contract](../launchers/index.md).

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant W as WMI service
    participant P as Target process
    B->>W: Open root\cimv2 session
    B->>W: Invoke Win32_Process.Create
    W->>P: Launch command line
    W-->>B: Return process identifier
```

## Requirements

- Local administrator on the target
- WMI/DCOM via TCP 135 and negotiated dynamic RPC ports
- Referenced files already present on the target

## Cleanup

The returned PID identifies the process created by WMI. Clean up files and
other changes made by the command. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes the WMI process-creation call and the resulting
process and command line.

