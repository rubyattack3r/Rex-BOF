# Standalone Windows CLI

The standalone bundle contains `rex.x64.exe` and `rex.x86.exe`. Both use the
same technique implementations, lifecycle, result format, and recovery records
as the BOFs. Choose the executable matching the local Windows process
architecture; target-side DLL contracts must also match the selected technique
architecture where documented.

## Commands

```text
rex.x64.exe preflight wmi-exec --target server01 --command "whoami.exe /all"
rex.x64.exe wmi-exec --target server01 --command "whoami.exe /all"
rex.x64.exe wsman-exec --target server01 --program cmd.exe --arguments "/c whoami"
```

The CLI exposes the primitive options listed in the
[technique reference](../techniques/README.md). It does not reproduce the CNA
launcher registry; compose the documented [launcher contract](../launchers/README.md)
as the technique's command, program, and argument values.

## Authentication

The CLI uses its process identity by default. Supply a down-level or UPN
principal to create command-scoped network credentials:

```text
rex.x64.exe preflight wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token DOMAIN\operator --password <password>

rex.x64.exe wmi-exec --target server01 --command "whoami.exe /all" \
  --make-token operator@example.com
Password:
```

When `--password` is omitted, Rex reads it from an interactive Windows console
with echo disabled. Redirected input is not accepted for the prompt. Inline
passwords can be retained in shell history or visible to process-inspection
tools. The prompt avoids the command line, but the credential still exists in
the Rex process while the command runs.

Rex applies the new-credentials token on a joined worker thread, runs decode,
preflight, execution, and release there, then reverts impersonation and closes
the token before printing the result. The process token is unchanged. Windows
may defer password validation until the first remote access.

## Results

Exit status is `0` for a successful Rex result, `1` for a technique or identity
failure, and `2` for CLI usage errors. Standard output contains the same typed
records described in [transcript output](../output.md); usage and local parsing
errors go to standard error.
