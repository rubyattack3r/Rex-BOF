# Managed .NET launchers

See the [launcher index](index.md) for selector inventory and shared evidence,
preflight, and recovery boundaries.

`RegAsm.exe`, `RegSvcs.exe`, and `InstallUtil.exe` consume .NET Framework
assemblies, but they do not share one generic managed-payload contract. The
assembly must implement the contract for the selected launcher. It cannot be a
native DLL, a modern .NET-only application, or an arbitrary managed library.

The built-in descriptors use the 64-bit .NET Framework v4 tools under
`C:\Windows\Microsoft.NET\Framework64\v4.0.30319`. Use
`--launcher-program` with the corresponding `Framework` path for a 32-bit
assembly or with another verified target-resident path when the environment
requires it.

## RegAsm

`RegAsm.exe` registers COM-visible managed types. Mark the intended class
`[ComVisible(true)]`, give it a stable GUID, and expose static registration
callbacks when the assembly needs explicit registration or unregistration
behavior:

```csharp
using System;
using System.Runtime.InteropServices;

[ComVisible(true)]
[Guid("01234567-89AB-CDEF-0123-456789ABCDEF")]
public sealed class ExampleComponent
{
    [ComRegisterFunction]
    public static void Register(Type type)
    {
        // Perform the documented registration action.
    }

    [ComUnregisterFunction]
    public static void Unregister(Type type)
    {
        // Reverse the registration action.
    }
}
```

Use `RegAsm.exe` for registration and `RegAsm.exe (/U)` for unregistration.
Callbacks should be idempotent and should fail clearly rather than leaving an
ambiguous partial registration.

```text
rex wmi-exec --target 10.0.0.15 --payload-type managed \
  --remote-path C:\ProgramData\component.dll --launcher RegAsm.exe

rex wmi-exec --target 10.0.0.15 --payload-type managed \
  --remote-path C:\ProgramData\component.dll --launcher "RegAsm.exe (/U)"
```

## RegSvcs

`RegSvcs.exe` registers a .NET Framework Enterprise Services component. The
assembly must be strong-named, reference `System.EnterpriseServices`, and
expose a component derived from `ServicedComponent`. Supply the application and
activation metadata required by the intended COM+ registration:

```csharp
using System.EnterpriseServices;
using System.Runtime.InteropServices;

[assembly: ApplicationName("ExampleApplication")]
[assembly: ApplicationActivation(ActivationOption.Library)]

[ComVisible(true)]
[Guid("01234567-89AB-CDEF-0123-456789ABCDEF")]
public sealed class ExampleServicedComponent : ServicedComponent
{
}
```

Sign the final assembly and verify the strong name before using
`RegSvcs.exe`. The `RegSvcs.exe (/U)` selector requests unregistration, but any
additional state created by component callbacks remains payload-defined.

```text
rex wmi-exec --target 10.0.0.15 --payload-type managed \
  --remote-path C:\ProgramData\serviced-component.dll --launcher RegSvcs.exe
```

## InstallUtil

`InstallUtil.exe` discovers public installer classes derived from
`System.Configuration.Install.Installer` and marked `[RunInstaller(true)]`.
Implement install and uninstall behavior as a pair. If the installer overrides
`Commit` or `Rollback`, define those paths consistently as well:

```csharp
using System.Collections;
using System.ComponentModel;
using System.Configuration.Install;

[RunInstaller(true)]
public sealed class ExampleInstaller : Installer
{
    public override void Install(IDictionary stateSaver)
    {
        base.Install(stateSaver);
        // Perform the documented install action.
    }

    public override void Uninstall(IDictionary savedState)
    {
        // Reverse the install action.
        base.Uninstall(savedState);
    }
}
```

The built-in descriptor suppresses the default log file and console logging.
Use `InstallUtil.exe (/U)` for the uninstall path.

```text
rex wsman-exec --target 10.0.0.15 --payload-type managed \
  --remote-path C:\ProgramData\installer.dll --launcher InstallUtil.exe
```

## Build and validation checklist

Validate the final artifact rather than trusting its extension or source
project:

1. Target .NET Framework, not modern .NET alone.
2. Match the assembly architecture to the selected Framework or Framework64
   launcher.
3. Confirm every referenced assembly exists on the target.
4. Inspect metadata, attributes, strong-name state, and assembly references
   with `corflags.exe`, `ildasm.exe`, or an equivalent metadata reader.
5. Test registration and unregistration, or installation and uninstallation,
   as a pair on a disposable host.
6. Make callbacks safe to repeat and document how partial failure is reversed.

`rex preflight` validates the remote execution technique, not the managed
assembly. It does not read the remote file, inspect CLR metadata, verify a
strong name, or prove that the selected callback will run.

## Operator recovery

The BOF reports that the remote process was dispatched or completed according
to the selected technique. It does not infer registry, COM+, installer,
service, or file changes made by managed callbacks. Record and reverse those
payload-defined changes separately, then remove the remote assembly after the
launcher releases it.

## Detection considerations

Managed registration and installer execution are observable through process
creation, command-line, CLR loading, registry or COM+ changes, and any
payload-defined side effects. No selector is undetectable.

References: [`ComRegisterFunctionAttribute`](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.comregisterfunctionattribute),
[`ServicedComponent`](https://learn.microsoft.com/en-us/dotnet/api/system.enterpriseservices.servicedcomponent),
[`Installer`](https://learn.microsoft.com/en-us/dotnet/api/system.configuration.install.installer),
[`RegAsm`](https://learn.microsoft.com/en-us/dotnet/framework/tools/regasm-exe-assembly-registration-tool),
[`RegSvcs`](https://learn.microsoft.com/en-us/dotnet/framework/tools/regsvcs-exe-net-services-installation-tool),
and [`InstallUtil`](https://learn.microsoft.com/en-us/dotnet/framework/tools/installutil-exe-installer-tool).
