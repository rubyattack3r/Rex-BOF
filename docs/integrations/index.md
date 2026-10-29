# Integrations

Rex ships the same x86 and x64 BOFs for Cobalt Strike and Sliver. Keep each
integration file with the objects from its release bundle.

| Integration | Operator surface | Architecture names | Launcher handling |
|---|---|---|---|
| [Cobalt Strike](cobalt-strike.md) | `rex.cna` registers one `rex` command | `x86`, `x64` | CNA resolves built-in and custom launcher contracts |
| [Sliver](sliver.md) | `extension.json` registers eight `rex-*` commands | `386`, `amd64` | Primitive program and argument fields are supplied directly |

Both integrations use the active implant token, support preflight, and operate
on one target per command.
