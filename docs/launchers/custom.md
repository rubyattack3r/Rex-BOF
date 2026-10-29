# Custom launchers

Use `custom` for a target-resident executable that is not in the built-in
launcher registry.

## Selecting a candidate

The [LOLBAS catalog](https://lolbas-project.github.io/) is a useful starting
point. Before adding a candidate, confirm that:

1. the exact executable and required features exist on the target;
2. its function accepts `dll`, `cpl`, or `managed` input as declared;
3. its invocation fits one single-line template;
4. payload, launcher, and process architectures are compatible; and
5. its side effects and cleanup are understood.

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

The CNA validates template syntax. Verify the program, payload, ABI, and any
PID on the target.

## Validation and cleanup

Document accepted payload formats, architectures, quoting rules, dependencies,
exit-status meaning, persistent effects, and recovery beside the launcher.
Test the rendered invocation on a disposable host and document cleanup.

## Detection

Document expected process, command-line, module-load, and side-effect telemetry
for the selected program.
