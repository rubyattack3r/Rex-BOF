# Identity and command-scoped credentials

Rex uses the current process or implant token unless `--make-token` and
`--password` are supplied. Make-token credentials apply to one Rex command.
Rex creates a `LOGON32_LOGON_NEW_CREDENTIALS` token, impersonates it on a
joined worker, runs the complete preflight or execution lifecycle, reverts the
worker, and closes the token. The process or implant token does not change.

Windows may not validate a new-credentials password until Rex reaches the
remote API. A successful local logon call therefore does not prove that the
credential can access the target.

## Authentication identity and execution identity

The account that authenticates a remote API is not always the account that
runs the target-side program:

- WMI process creation and WinRM normally launch under the authenticated
  account.
- Existing services keep the account already configured on the service.
- Rex-created services and tasks run as `LocalSystem`.
- WMI provider DLLs and permanent command consumers run in WMI-managed
  `LocalSystem` hosts.
- `dcom-ishell-dispatch2` launches through the target's interactive Explorer
  session, so the program runs as that signed-in user.

Make-token changes remote authentication. It does not override these
target-side execution rules.

## Validation evidence

The per-technique command-scoped identity results, environments, and
architecture coverage are recorded in [tested environments](platforms.md).
Those tests used benign target-side fixtures to record the execution account
and confirmed that the implant identity was unchanged after each command.

The DCOM validation used an active domain-user Explorer session on a member
server. Both the same account and a different valid administrator passed
preflight; execution ran as the interactive user. No AppID or machine-wide
DCOM permission changed. Earlier controls using a built-in Administrator
Explorer session returned access denied, so preflight remains necessary for
the intended session and target. A disconnected RDP session returned
`0x8000401A` until it was made active again.

## Operator considerations

- The temporary identity needs the privileges listed in the selected
  [technique guide](techniques/README.md).
- Preflight uses the same identity and request as execution, but it cannot
  guarantee that a later event, task, service, or payload will complete.
- Cobalt Strike and Sliver cannot prompt securely inside a BOF. Their password
  may remain in console, server, tasking, or audit history. The standalone CLI
  can prompt with echo disabled.
- Rex clears its local packed password buffer and does not put credentials in
  transcript or recovery output.
