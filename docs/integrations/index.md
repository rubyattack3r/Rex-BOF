# Integrations

Rex ships the same x86 and x64 technique objects through separate Cobalt
Strike and Sliver operator surfaces. Keep an integration file and its objects
from the same release bundle.

| Integration | Operator surface | Architecture names | Launcher handling |
|---|---|---|---|
| [Cobalt Strike](cobalt-strike.md) | `rex.cna` registers one `rex` command | `x86`, `x64` | CNA resolves built-in and custom launcher contracts |
| [Sliver](sliver.md) | `extension.json` registers eight `rex-*` commands | `386`, `amd64` | Primitive program and argument fields are supplied directly |

Both integrations inherit the active implant token, support read-only
preflight, operate on one target per invocation, and enter the same native
decode, execution, result, recovery, and cleanup paths.
