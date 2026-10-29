# Custom launchers

The reserved `custom` selector describes a target-resident executable without
adding it to the built-in CNA registry. The operator owns the payload ABI,
success semantics, dependencies, side effects, and recovery procedure.

## Selecting a candidate

Use the [LOLBAS catalog](https://lolbas-project.github.io/) to research an
execution function compatible with the target environment and payload format.
A catalog entry is a candidate, not proof that it exists or works on a specific
host. Confirm that:

1. the exact executable and required features exist on the target;
2. its function accepts `dll`, `cpl`, or `managed` input as declared;
3. its invocation fits one single-line template;
4. payload, launcher, and process architectures are compatible; and
5. the engagement permits its side effects and telemetry.

A function limited to download, copy, compile, or another non-execution action
is not automatically a launcher.

## Template contract

Custom launchers require:

- `--launcher custom`;
- an absolute drive path in `--launcher-program`;
- a single-line `--launcher-arguments` containing `{remote_path}`; and
- `--entry-point` or `--process-id` when its corresponding placeholder appears.

Unknown or unexpanded placeholders are rejected. Literal braces are not
supported in custom templates. The CNA quotes a substituted remote path when it
contains spaces or tabs; the remaining parsing behavior belongs to the custom
program.

```text
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path "D:\Approved Staging\payload.dll" \
  --launcher custom \
  --launcher-program "D:\Approved Tools\loader-host.exe" \
  --launcher-arguments "--module {remote_path} --export {entry_point}" \
  --entry-point Run
```

For a process-bound contract:

```text
rex wmi-exec --target 10.0.0.15 --payload-type dll \
  --remote-path D:\Staging\payload.dll \
  --launcher custom \
  --launcher-program D:\Tools\injector.exe \
  --launcher-arguments "--pid {process_id} --module {remote_path}" \
  --process-id 4242
```

The CNA validates syntax only. It does not check that the program or payload
exists, that the executable implements the claimed contract, or that a PID is
valid. `rex preflight` checks the selected remote-execution technique, not the
custom launcher.

## Validation and recovery

Document accepted payload formats, architectures, quoting rules, dependencies,
exit-status meaning, persistent effects, and recovery beside the launcher.
Test the exact rendered invocation on a disposable host. Track and reverse
payload and launcher side effects separately from technique-owned recovery.

## Detection considerations

Custom means operator-defined, not trusted or low-observability. Process creation, command lines, module loads, and launcher-defined side effects remain detectable.
