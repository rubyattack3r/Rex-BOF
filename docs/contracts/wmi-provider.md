# WMI provider DLL contract

The three WMI provider selectors load a native in-process COM server through
WMI. They do not invoke a DLL export as a launcher would.

| Selector | Registration created by Rex | DLL responsibility |
|---|---|---|
| `wmi-provider` | `__Win32Provider` | Initialize through `IWbemProviderInit` |
| `wmi-instance-provider` | `__Win32Provider` and `__InstanceProviderRegistration` | Initialize and implement the advertised instance operations |
| `wmi-method-provider` | `__Win32Provider` and `__MethodProviderRegistration` | Initialize and implement method dispatch |

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

Instance and method providers must also expose `IWbemServices` from the
provider object and implement the operations associated with their role. Rex's
instance registration advertises `GetObjectAsync` and
`CreateInstanceEnumAsync`; put and delete support are disabled. A method
provider must implement `ExecMethodAsync` for the methods declared by its
managed classes. Unsupported `IWbemServices` operations should return the
appropriate not-supported or provider-not-capable result.

Rex writes the COM and WMI provider registration itself, so
`DllRegisterServer` and `DllUnregisterServer` are not required.

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

It then calls `MSFT_Providers.Load` for that provider. The selector determines
the role registration:

- `wmi-provider` creates no provider-role registration;
- `wmi-instance-provider` creates `__InstanceProviderRegistration` with
  `SupportsGet=True`, `SupportsEnumeration=True`, `SupportsPut=False`, and
  `SupportsDelete=False`; and
- `wmi-method-provider` creates `__MethodProviderRegistration`.

Rex does not create class, property, event, or event-consumer provider roles.

## Managed-class schema

Role registration identifies a provider's capabilities, but it does not
associate an arbitrary class with that provider. The target must already have
a compatible managed-class definition in `root\default`:

- the class needs `Dynamic` and `Provider` qualifiers naming the provider;
- an instance provider's class and DLL must agree on the retrieval and
  enumeration behavior; and
- a method provider's class must declare the methods implemented by the DLL,
  including their input, output, `Implemented`, and static or instance
  semantics.

Rex does not compile MOF or create, modify, validate, or remove these class
definitions. Treat the schema and DLL as one versioned target-side contract.

See Microsoft documentation for
[`__InstanceProviderRegistration`](https://learn.microsoft.com/en-us/windows/win32/wmisdk/--instanceproviderregistration),
[writing an instance provider](https://learn.microsoft.com/en-us/windows/win32/wmisdk/writing-an-instance-provider),
and [registering a method provider](https://learn.microsoft.com/en-us/windows/win32/wmisdk/registering-a-method-provider).

## Validation

Before execution:

1. Place the DLL at an absolute local drive path on the target.
2. Inspect its PE architecture, `DllGetClassObject` and `DllCanUnloadNow`
   exports, dependent DLLs, and embedded provider CLSID.
3. Confirm that creating the CLSID yields an object supporting
   `IWbemProviderInit` and that initialization calls `SetStatus`.
4. For an instance or method role, confirm that the same object exposes
   `IWbemServices`, implements every advertised operation, and matches the
   installed managed-class schema.
5. Test load, role dispatch, and unload together and verify that the provider
   host releases the file before deletion.

Rex preflight confirms only that the provider name, selected role registration,
and architecture-specific COM registration are unused. It does not open the
DLL or inspect its exports, CLSID, dependencies, interfaces, managed-class
schema, or behavior.
