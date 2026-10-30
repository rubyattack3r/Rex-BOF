# dcom-ishell-dispatch2

## How it works

Rex activates the remote Shell COM class, gets `IShellDispatch2` from an
interactive Explorer session, and calls `ShellExecute`. Cobalt Strike can
supply the program through a [launcher contract](../launchers/README.md).

An `explorer.exe` process alone is not enough. Preflight must resolve the Shell
automation interface in a usable session. During Server 2025 validation, a
disconnected RDP session returned `0x8000401A`; reactivating that session allowed
the same request to pass.

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant D as DCOM
    participant U as Interactive shell
    participant P as Target process
    B->>D: Activate remote ShellWindows object
    D->>U: Resolve IShellDispatch2
    B->>U: Invoke ShellExecute
    U->>P: Launch program and arguments
    B-->>D: Release COM interfaces
```

## Identity

The current or make-token identity is used for DCOM activation. `ShellExecute`
runs through the target's Explorer session, so the program inherits that
interactive user's identity rather than the make-token identity. Server 2025
validation passed with both the same account and a different valid
administrator. No AppID or machine-wide DCOM permission changed. See the
[identity matrix](../identity.md) and [tested environments](../platforms.md).

## Requirements

- Local administrator on the target
- Active interactive user logon with a usable Explorer shell
- DCOM via TCP 135 and negotiated dynamic RPC ports
- Referenced files already present on the target

## Cleanup

Delete payload files after the launched process releases them, and clean up
changes made by the program. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes DCOM activation, `ShellExecute`, and the resulting
process.

