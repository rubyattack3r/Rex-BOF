# Authoring and validating payloads

A file extension does not define a launcher payload. The final artifact must
match the required format, architecture, ABI, dependencies, and callback
behavior.

## Contract map

| Launcher family | Payload | Required implementation |
| --- | --- | --- |
| Standard `rundll32.exe` | Native DLL | Export named by `--entry-point`, using the Rundll32 callback ABI |
| Advpack, `msiexec /y`, `odbcconf`, ordinary `regsvr32` | Native DLL | `DllRegisterServer(void)` returning `HRESULT` |
| `msiexec /z`, `regsvr32 /u` | Native DLL | `DllUnregisterServer(void)` returning `HRESULT` |
| `regsvr32 /i` variants | Native DLL | `DllInstall(BOOL install, LPCWSTR commandLine)` |
| `control.exe` | Native CPL module | Exported `CPlApplet(HWND, UINT, LPARAM, LPARAM)` callback |
| `mavinject.exe` | Native DLL | Loader-safe `DllMain`; architecture matching the selected process |
| `RegAsm.exe` | Managed assembly | COM-visible types and registration callbacks when required |
| `RegSvcs.exe` | Strong-named managed assembly | Enterprise Services component derived from `ServicedComponent` |
| `InstallUtil.exe` | Managed assembly | Public `Installer` marked `[RunInstaller(true)]` |
| `custom` | Operator-defined | Contract implemented by the selected target-resident executable |

## Inspect the artifact

```powershell
dumpbin.exe /headers .\payload.dll
dumpbin.exe /exports .\payload.dll
corflags.exe .\managed-payload.dll
ildasm.exe /text .\managed-payload.dll
```

On a non-Windows build host, `llvm-objdump -p`, a MinGW `objdump -p`, or
`file` can confirm PE architecture and native exports. Use a managed metadata
reader for assembly references, attributes, target framework, and strong-name
state.

Before placing the artifact on the target:

1. The payload type matches the launcher family.
2. The file already exists at the exact absolute `--remote-path`.
3. Payload, launcher, BOF, and target-process architectures are compatible.
4. Required exports have exact undecorated names and calling conventions.
5. The selected CLR and every dependent assembly exist on the target.
6. Callbacks return meaningful failure status and tolerate repeated invocation.
7. Record its side effects and cleanup steps.
8. A disposable-host test proves the intended callback runs and the module can
   be released before deletion.

`rex preflight` checks the execution technique, not the payload ABI.

See the [launcher index](index.md) and the family page matching the artifact.
