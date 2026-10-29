# Tested platforms

The following results were observed on 2026-10-28 with matching x86 and x64 BOFs and benign fixtures. They are evidence from one authorized environment, not guarantees for other Windows builds, identity policies, endpoint controls, or network baselines. An unlisted platform is untested, not necessarily unsupported.

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

The transcript encoder is exercised separately by `tests/text_test.c` on Windows CI, covering whitespace, delimiters, control bytes, paths, Japanese text, sentinels, null values, and invalid UTF-16.

`DCOMIShellDispatch2` requires a usable interactive shell, not merely an `explorer.exe` process. `WMIProvider` requires a provider DLL whose architecture and compiled CLSID match the BOF object and requested registration. These preconditions materially affect whether the techniques can execute in another environment.

