# WMIProvider

## Mechanism and prerequisites

Registers COM metadata and a `__Win32Provider` instance, then invokes `MSFT_Providers.Load`. It requires local-administrator rights, reachable WMI/DCOM, an unused provider name and CLSID registration, and a provider DLL that already exists on the target. The DLL's architecture must match the selected BOF object and its `DllGetClassObject` implementation must expose the value supplied through `--provider-clsid`.

Payload support: an already-staged native WMI provider DLL only. This technique does not use a launcher because the provider host loads the registered DLL directly.

```mermaid
sequenceDiagram
    participant B as rex BOF
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

## Privilege and network surface

- Local administrator on the target
- WMI/DCOM via TCP 135 and negotiated dynamic RPC ports
- Remote registry access through the WMI and registry interfaces used by the active token
- No BOF-managed SMB delivery; the DLL must already exist remotely

## Operator recovery

Call `MSFT_Providers.UnLoad`, delete the exact `__Win32Provider` instance, and remove the reported `InprocServer32` key and CLSID parent. Wait for the provider host to release the module before deleting the separately delivered DLL. See [operator recovery](../recovery.md).

## Detection considerations

Provider registration, registry changes, WMI activity, and an unusual provider-host DLL load are well-signatured. Provider naming and hosting choices can change alert confidence, but the technique is never undetectable.

