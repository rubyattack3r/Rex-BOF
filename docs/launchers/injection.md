# Process-bound injection

## Mechanism and prerequisites

The `mavinject.exe` selector asks a target-resident `mavinject.exe` to load a
native DLL into an existing process. It requires an explicit positive decimal
`--process-id`; `rex` does not discover or select a process.

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

The CNA validates only that `--process-id` is a positive decimal value and
renders it into the launcher arguments. `rex preflight` does not open the PID,
resolve its image, compare architectures, or verify payload presence. Any PID
returned by `WMIExec` is the newly created `mavinject.exe` process, not the
process supplied with `--process-id`.

## Operator recovery

Injection cannot be generically undone. Recovery may require a payload-specific
unload operation or termination of the approved disposable process before the
remote DLL can be removed.

## Detection considerations

Cross-process access and module loading are highly observable. A dispatch result proves launcher acceptance, not successful DLL initialization.
