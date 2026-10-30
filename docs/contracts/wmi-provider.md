# WMI provider DLL contract

`wmi-provider` loads a native in-process COM server through WMI. It does not
invoke a DLL export as a launcher would.

## Required contract

The DLL must:

1. Match the architecture of the selected BOF and WMI provider host. An x64
   BOF writes the 64-bit COM registration; an x86 BOF writes the 32-bit view.
2. Export `DllGetClassObject`. For the exact CLSID passed through
   `--provider-clsid`, it must return an `IClassFactory` that creates the WMI
   provider object.
3. Implement `IUnknown` and `IWbemProviderInit` on that object. Its
   `Initialize` method must report success or failure through
   `IWbemProviderInitSink::SetStatus`.
4. Support the registered `ThreadingModel=Both` behavior and keep `DllMain`
   free of work that can block under loader lock.
5. Resolve all native dependencies inside the provider host and release its
   COM references cleanly. Export `DllCanUnloadNow` so COM can determine when
   the module is safe to unload.

Rex writes the COM and WMI registration itself, so `DllRegisterServer` and
`DllUnregisterServer` are not required.

See Microsoft documentation for
[`DllGetClassObject`](https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-dllgetclassobject),
[`IClassFactory`](https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nn-unknwn-iclassfactory),
and [provider initialization](https://learn.microsoft.com/en-us/windows/win32/wmisdk/initializing-a-provider).

## Registration performed by Rex

Rex writes the target-side DLL path under
`HKLM\Software\Classes\CLSID\{CLSID}\InprocServer32`, sets
`ThreadingModel=Both`, and creates a `__Win32Provider` instance in
`root\default` with:

- `Name` set from `--provider-name`;
- `Clsid` set from `--provider-clsid`; and
- `HostingModel` set to `LocalSystemHost:<provider-name>`.

It then calls `MSFT_Providers.Load` for that provider. Rex does not create an
`__InstanceProviderRegistration`, `__MethodProviderRegistration`, or another
provider-role registration. A DLL intended to serve WMI classes, methods,
properties, or events needs the corresponding interface and role registration
in addition to this load contract.

## Validation

Before execution:

1. Place the DLL at an absolute local drive path on the target.
2. Inspect its PE architecture, `DllGetClassObject` and `DllCanUnloadNow`
   exports, dependent DLLs, and embedded provider CLSID.
3. Confirm that creating the CLSID yields an object supporting
   `IWbemProviderInit` and that initialization calls `SetStatus`.
4. Test load and unload together and verify that the provider host releases
   the file before deletion.

Rex preflight confirms only that the provider name and architecture-specific
COM registration are unused. It does not open the DLL or inspect its exports,
CLSID, dependencies, or initialization behavior.
