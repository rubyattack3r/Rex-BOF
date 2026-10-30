# scm-change-service

## How it works

Rex uses Service Control Manager RPC to read and reconfigure an existing stopped
service. Preflight records its binary path and start type and confirms that it
is stopped.

The replacement executable must already be on the target and implement the
Windows service lifecycle.

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant C as SCM RPC
    participant S as Existing service
    B->>C: Read service configuration and state
    B->>C: Change binary path and start type
    C->>S: Start reconfigured service
    B-->>B: Emit original and applied configuration
```

## Service executable contract

The replacement must be a Windows service executable, not an ordinary console
program. It must connect to SCM, implement `ServiceMain`, report service-state
transitions, and run under the existing service identity. Its architecture
must be supported by the target but does not need to match the BOF.

See the complete [Windows service executable contract](../contracts/service-executable.md)
for lifecycle, identity, path, validation, and result requirements.

## Requirements

- Local administrator on the target
- SCM RPC, commonly over SMB on TCP 445
- RPC endpoint mapper on TCP 135 where required
- Contract-compliant service executable already present on the target

## Cleanup

Stop the service, confirm its current configuration, and restore the recorded
binary path and start type. Delete the executable after the service releases
it. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes SCM configuration changes, service starts, and
image paths.

