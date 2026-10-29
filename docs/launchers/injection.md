# Process-bound injection

## Requirements

`mavinject.exe` loads a native DLL into an existing process. Supply its PID with
`--process-id`; Rex does not select a process.

```text
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path C:\ProgramData\payload.dll \
  --launcher mavinject.exe --process-id 4242
```

Before execution:

1. Select a disposable process permitted by the engagement plan.
2. Confirm immediately before execution that the PID still names that process.
3. Match the DLL architecture to the target process and `mavinject.exe`.
4. Keep `DllMain` loader-safe and make repeated loads harmless.
5. Confirm the process lifetime is sufficient for the intended validation.

The CNA only checks that `--process-id` is a positive decimal value. Any PID
returned by `wmi-exec` identifies the new `mavinject.exe` process, not the
process receiving the DLL.

## Cleanup

Unload the DLL with its own cleanup path or stop the disposable process before
deleting the file.

## Detection

Relevant telemetry includes cross-process access and module loading.
