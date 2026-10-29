# wmi-provider

## How it works

Rex registers COM metadata and a `__Win32Provider` instance, then calls
`MSFT_Providers.Load`. The provider DLL must already be on the target, match the
BOF architecture, and expose the CLSID supplied through `--provider-clsid`.

The provider host loads the DLL directly; this technique does not use a
launcher.

```mermaid
sequenceDiagram
    participant B as Rex BOF
    participant R as Remote registry
    participant W as WMI service
    participant H as Provider host
    B->>W: Verify provider name is unused
    B->>R: Verify CLSID registration is unused
    B->>R: Create CLSID and InprocServer32 keys
    B->>W: Create __Win32Provider instance
    B->>W: Invoke MSFT_Providers.Load
    W->>H: Load provider DLL
    B-->>B: Emit provider recovery context
```

## Requirements

- Local administrator on the target
- WMI/DCOM via TCP 135 and negotiated dynamic RPC ports
- Remote registry access through the WMI and registry interfaces used by the active token
- Provider DLL already present on the target

## Cleanup

Call `MSFT_Providers.UnLoad`, delete the reported `__Win32Provider` instance,
and remove its `InprocServer32` and CLSID keys. Delete the DLL after the
provider host releases it. See [recovery](../recovery.md).

## Detection

Relevant telemetry includes provider registration, registry changes, WMI
activity, and provider-host DLL loads.

