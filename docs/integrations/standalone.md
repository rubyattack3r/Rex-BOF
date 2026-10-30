# Standalone Windows CLI

The standalone bundle contains `rex.x64.exe` and `rex.x86.exe`. Both use the
same technique implementations, lifecycle, result format, and recovery records
as the BOFs. Choose the executable matching the local Windows process
architecture; target-side DLL contracts must also match the selected technique
architecture where documented.

## Help

Help is built into both executables:

```text
rex.x64.exe help
rex.x64.exe --help
rex.x64.exe techniques
rex.x64.exe help wmi-exec
rex.x64.exe wmi-exec --help
```

`help` shows the command forms and identity options. `techniques` lists every
selector with a short description. Technique help shows the exact required and
optional fields accepted by the executable.

## Commands

```text
rex.x64.exe preflight wmi-exec --target server01 --command "whoami.exe /all"
rex.x64.exe wmi-exec --target server01 --command "whoami.exe /all"
rex.x64.exe wsman-exec --target server01 --program cmd.exe --arguments "/c whoami"
```

The CLI accepts the primitive fields listed in the
[technique reference](../techniques/README.md). It does not resolve the CNA's
launcher selectors. Supply the resulting command, program, and arguments
directly when using a documented [launcher contract](../launchers/README.md).

## Authentication

The CLI uses its process identity by default. Supply a down-level or UPN
principal to create command-scoped network credentials:

```text
rex.x64.exe preflight wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token DOMAIN\operator --password <password>

rex.x64.exe wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token operator@example.com --password <password>

rex.x64.exe wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token operator@example.com
Password:
```

When `--password` is omitted, Rex reads it from an interactive Windows console
with echo disabled. Redirected input is not accepted for the prompt. Inline
passwords can be retained in shell history or visible to process-inspection
tools. The prompt avoids the command line, but the credential still exists in
the Rex process while the command runs.

Rex applies the new-credentials token on a joined worker thread. That thread
runs the complete command lifecycle and reverts impersonation before Rex
closes the token and prints the result. The process token remains unchanged.
Windows may defer password validation until the first remote access. The
target-side execution account depends on the technique; see
[identity and command-scoped credentials](../identity.md).

## Results

Exit status is `0` for a successful Rex result, `1` for a technique or identity
failure, and `2` for CLI usage errors. Standard output contains the same typed
records described in [transcript output](../output.md); usage and local parsing
errors go to standard error.
