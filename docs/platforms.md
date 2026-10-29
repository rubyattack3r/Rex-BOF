# Tested platforms

The following results were observed on 2026-10-28 through Cobalt Strike with
matching x86 and x64 BOFs and benign fixtures. They are evidence from one
authorized environment, not guarantees for other Windows builds, identity
policies, endpoint controls, or network baselines. An unlisted platform or
integration is untested, not necessarily unsupported. The technique matrix
records Cobalt Strike evidence; separate Sliver runtime evidence follows it.

| Technique | Windows Server 2025 build 26100 x64 | Windows Server 2025 build 26100 x86 | Observed evidence |
|---|---|---|---|
| [WMIProvider](techniques/wmiprovider.md) | Passed | Passed | Provider load completed with an architecture-matched DLL. |
| [WMIService](techniques/wmiservice.md) | Passed | Passed | Service start dispatched and original configuration recorded. |
| [WMIExec](techniques/wmiexec.md) | Passed | Passed | Process creation returned a PID and the benign marker was observed. |
| [WSManExec](techniques/wsmanexec.md) | Passed | Passed | Shell command completed and the benign marker was observed. |
| [TaskSchedExec](techniques/taskschedexec.md) | Passed | Passed | Task dispatched, completed with result `0`, and deletion context was emitted. |
| [DCOMIShellDispatch2](techniques/dcomishelldispatch2.md) | Passed | Passed | Shell execution dispatched with a usable interactive Explorer session. |
| [SCMChangeService](techniques/scmchangeservice.md) | Passed | Passed | Service start dispatched and original configuration recorded. |
| [SCMCreateService](techniques/scmcreateservice.md) | Passed | Passed | Service start dispatched and deletion context was emitted. |

Every built-in launcher selector and the custom launcher contract were validated through x64 `WMIExec`. The `rundll32.exe` contract was also validated through `WSManExec`, `TaskSchedExec`, and `DCOMIShellDispatch2`. Validation used payloads matching each launcher's native DLL, CPL, managed-assembly, or target-process contract.

On 2026-10-29, a clean Sliver release bundle was installed with the Sliver
v1.7.8 client connected to an isolated loopback server. The client registered
all eight commands and both `windows/386` and `windows/amd64` artifacts. The
Sliver v1.7.9 extension code then parsed the packaged manifest, resolved every
object, packed default `execute` and explicit `preflight` arguments, selected
Reflektor and `coff-loader` fallback paths, and built Reflektor requests for
every command and architecture. The v1.7.9 Reflektor COFF parser also accepted
all 16 objects and found each exact architecture-specific entry point.

Official Sliver v1.7.9 x86 and x64 Windows implants then executed the packaged
`rex_wmiexec.<arch>.o` object through built-in Reflektor on Windows Server 2025 build
26100. Both architectures returned a successful `validated` preflight without
creating a marker. Both then returned a WMI process identifier from `execute`,
and their architecture-specific benign markers were independently observed.
This is live evidence for WMIExec and the Reflektor loader path, not for every
Sliver command or the declared `coff-loader` fallback. The technique matrix
above remains Cobalt Strike evidence.

The transcript encoder is exercised separately by `tests/text_test.c` on Windows CI, covering whitespace, delimiters, control bytes, paths, Japanese text, sentinels, null values, and invalid UTF-16.

`DCOMIShellDispatch2` requires a usable interactive shell, not merely an `explorer.exe` process. `WMIProvider` requires a provider DLL whose architecture and compiled CLSID match the BOF object and requested registration. These preconditions materially affect whether the techniques can execute in another environment.

