# Rex BOF

Rex packages twelve Windows remote-execution techniques as native x86 and x64
Beacon Object Files for Cobalt Strike and Sliver, plus standalone Windows
executables built from the same runtime.

<picture>
  <source media="(prefers-color-scheme: light)" srcset="docs/demo/demo-light.svg">
  <img alt="rex dispatching wmi-exec and reporting its remote process identifier" src="docs/demo/demo-dark.svg">
</picture>

## Install

Download and extract a bundle from
[Releases](https://github.com/rubyattack3r/Rex-BOF/releases):

- `Rex-BOF-<version>.zip`: Cobalt Strike CNA and all x86/x64 objects.
- `Rex-BOF-Sliver-<version>.zip`: Sliver extension manifest and the same
  x86/x64 objects.
- `Rex-BOF-Standalone-<version>.zip`: x86 and x64 `rex.exe` command-line
  executables.

Each bundle includes offline documentation and has a `.sha256` checksum and a
build provenance attestation:

```sh
gh attestation verify <bundle>.zip -R rubyattack3r/Rex-BOF
```

For Cobalt Strike, keep `rex.cna` and all objects together and load `rex.cna`
in Script Manager. For Sliver, install the extracted directory:

```text
extensions install /path/to/Rex-BOF-Sliver-<version>
```

## Usage

```text
# Cobalt Strike
rex help
rex preflight wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
rex wmi-exec --target 10.0.0.15 --command "whoami.exe /all"

# Sliver
rex-wmi-exec --action preflight --target 10.0.0.15 --command "whoami.exe /all"
rex-wmi-exec --target 10.0.0.15 --command "whoami.exe /all"

# Standalone Windows CLI
rex.x64.exe preflight wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
rex.x64.exe wmi-exec --target 10.0.0.15 --command "whoami.exe /all"
```

Each command targets one host and uses the current process or implant identity
by default. Add `--make-token DOMAIN\user --password <password>` or
`--make-token user@domain --password <password>` to use command-scoped network
credentials. The standalone CLI can prompt when `--password` is omitted. Use
`preflight` to check technique requirements without making the technique's
remote change. Payload files must already be on the target.

See the [operator documentation](docs/README.md),
[integration guides](docs/integrations/README.md),
[technique reference](docs/techniques/README.md),
[tested platforms](docs/platforms.md), and [cleanup guide](docs/recovery.md).

## Build from source

```sh
nix develop -c make verify
nix develop -c make release VERSION=v0.4.0
```

Build output is written beside each integration under `integrations/`. Release
ZIPs and checksums are written to `release/`.

Render the demo after editing `docs/demo/demo.toml` with:

```sh
nix develop .#demo -c make demo
```

## Responsible use

Use Rex only on systems you own or are authorized to test. See the
[Acceptable Use Policy](ACCEPTABLE_USE_POLICY.md).

## License

Copyright 2026 rubyattack3r. Licensed under the repository's
[MIT License](LICENSE).
