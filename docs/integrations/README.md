# Integrations

Rex ships the same x86 and x64 BOFs for Cobalt Strike and Sliver, plus a
standalone Windows CLI built from the shared runtime. Keep each integration
file with the objects from its release bundle.

| Integration | Operator surface | Architecture names | Launcher handling |
|---|---|---|---|
| [Cobalt Strike](cobalt-strike.md) | `rex.cna` registers one `rex` command | `x86`, `x64` | CNA resolves built-in and custom launcher contracts |
| [Sliver](sliver.md) | `extension.json` registers twelve `rex-*` commands | `386`, `amd64` | Primitive program and argument fields are supplied directly |
| [Standalone](standalone.md) | `rex.x64.exe` and `rex.x86.exe` | `x64`, `x86` | Primitive program and argument fields are supplied directly |

All three surfaces use the current identity by default, support optional
command-scoped network credentials and preflight, and operate on one target per
command.
