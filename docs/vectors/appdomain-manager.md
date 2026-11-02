# AppDomain-manager execution

This vector reuses a .NET Framework executable without replacing its service
or task action. When the process starts, the CLR reads the executable's
`.config` file. The configuration names a manager type and assembly, and the
CLR creates that manager early in process startup.

```text
existing task or service starts
        -> existing .NET executable starts
        -> CLR reads executable.config
        -> CLR loads the named assembly
        -> AppDomain manager runs
```

The executable path stays unchanged. The manager is loaded only when a new
process starts, so an already-running process must be stopped and started
again.

## Verified targets

| Existing object | .NET host | Start action | Execution proof |
|---|---|---|---|
| `\Microsoft\Windows\Server Initial Configuration Task` | `srvinitconfig.exe` | Enable and run the task | Manager ran inside `srvinitconfig.exe` as `SYSTEM` |
| `NetTcpPortSharing` | `SMSvcHost.exe` | Change to demand start and start the service | Manager ran inside `SMSvcHost.exe` as `NT AUTHORITY\LocalService` |

## Payload requirements

The assembly must:

- target a .NET Framework runtime compatible with the host;
- contain the exact manager type named in the configuration;
- implement the AppDomain-manager interface expected by the CLR;
- match the host architecture; and
- be readable by the task or service identity, including all dependencies.

## Workflow

1. Confirm the exact task or service and its executable path.
2. Save the existing `.config` file, or record that none exists.
3. Place the compatible manager assembly where the host can read it.
4. Add the manager assembly and manager type settings to the host's
   configuration.
5. Start a new host process using the exact task or service action listed
   above.
6. Confirm that the manager ran inside the expected process and identity.
7. Restore the original configuration and task or service state before
   removing the assembly.

Only the two target pairings above were reproduced. Do not infer that every
.NET task or service accepts this configuration.

## Detection and references

Watch for new or changed `.config` files beside Windows .NET executables,
unexpected assemblies loaded early in process startup, and unusual task or
service starts.

Microsoft documents the
[`appDomainManagerAssembly` element](https://learn.microsoft.com/dotnet/framework/configure-apps/file-schema/runtime/appdomainmanagerassembly-element)
and
[`appDomainManagerType` element](https://learn.microsoft.com/dotnet/framework/configure-apps/file-schema/runtime/appdomainmanagertype-element)
used by this startup behavior.
