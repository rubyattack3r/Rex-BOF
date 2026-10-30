# scm-create-service

## How it works

Rex creates and starts a service through SCM RPC. It implements the familiar
PsExec-style service pattern without using the PsExec utility.

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant C as SCM RPC
    participant S as Temporary service
    B->>C: Verify service name is unused
    B->>C: Create demand-start service
    C->>S: Start service
    B-->>B: Emit service recovery context
```

## Service executable contract

The image must be a Windows service executable, not an ordinary console
program. It must connect to SCM, implement `ServiceMain`, report service-state
transitions, and support clean stop handling. Rex creates it as a demand-start
own-process service under LocalSystem. Its architecture must be supported by
the target but does not need to match the BOF.

See the complete [Windows service executable contract](../contracts/service-executable.md)
for lifecycle, identity, path, validation, and result requirements.

## Identity

The current or make-token identity authenticates the SCM operations. Rex
creates the service under `LocalSystem`, so the service executable does not run
as the make-token account. See the [identity matrix](../identity.md).

## Requirements

- Local administrator on the target
- SCM RPC over SMB on TCP 445
- RPC endpoint mapper on TCP 135 where required
- Contract-compliant service executable already present on the target

## Cleanup

Stop and delete the reported service, then verify that it is gone. Delete the
executable after the service releases it. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes service creation, service start activity, the
service image path, and the separate file-transfer method.

