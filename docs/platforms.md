# Tested platforms

These tests used benign fixtures on Windows Server 2025 build 26100. The
Cobalt Strike matrix covers matching x86 and x64 BOFs. Other builds and
configurations have not been tested.

| Technique | Windows Server 2025 build 26100 x64 | Windows Server 2025 build 26100 x86 | Observed evidence |
|---|---|---|---|
| [`wmi-provider`](techniques/wmiprovider.md) | Passed | Passed | Provider loaded with an architecture-matched DLL. |
| [`wmi-service`](techniques/wmiservice.md) | Passed | Passed | Service start requested and original configuration recorded. |
| [`wmi-exec`](techniques/wmiexec.md) | Passed | Passed | Process creation returned a PID and the benign marker appeared. |
| [`wsman-exec`](techniques/wsmanexec.md) | Passed | Passed | Command returned status 0 and the benign marker appeared. |
| [`task-sched-exec`](techniques/taskschedexec.md) | Passed | Passed | Task returned result `0` and deletion data was recorded. |
| [`dcom-ishell-dispatch2`](techniques/dcomishelldispatch2.md) | Passed | Passed | Program started through an interactive Explorer session. |
| [`scm-change-service`](techniques/scmchangeservice.md) | Passed | Passed | Service start requested and original configuration recorded. |
| [`scm-create-service`](techniques/scmcreateservice.md) | Passed | Passed | Service start requested and deletion data was recorded. |

Every built-in launcher and the custom launcher were tested through x64
`wmi-exec`. `rundll32.exe` was also tested through `wsman-exec`,
`task-sched-exec`, and `dcom-ishell-dispatch2`. Each fixture implemented the
launcher's required DLL, CPL, managed-assembly, or process contract.

## Sliver

- Sliver v1.7.8 installed a clean release bundle and registered all eight
  commands for `windows/386` and `windows/amd64`.
- Sliver v1.7.9 parsed the manifest, packed `execute` and `preflight` requests,
  resolved all 16 objects and entry points, and built Reflektor requests.
- Official v1.7.9 x86 and x64 implants ran `rex_wmiexec.<arch>.o` through
  Reflektor on Server 2025 build 26100. Preflight created no marker. Execute
  returned a PID, and each architecture wrote its expected marker.
- The other seven commands and the `coff-loader` fallback have not been run
  through live Sliver implants.

Windows CI tests transcript encoding for whitespace, delimiters, control bytes,
paths, Japanese text, sentinels, null values, and invalid UTF-16.

`dcom-ishell-dispatch2` needs a usable interactive shell, not just an
`explorer.exe` process. `wmi-provider` needs a provider DLL with the same
architecture as the BOF and the requested CLSID.

