# Contributing

By contributing, you agree to the [Code of Conduct](CODE_OF_CONDUCT.md) and
[Acceptable Use Policy](../ACCEPTABLE_USE_POLICY.md).

Before opening a pull request:

- Use a focused branch.
- Run `nix develop -c make verify`.
- Add tests and documentation for behavior changes.
- Build and inspect both architectures.
- Do not commit generated objects, payloads, target configuration, credentials,
  keys, or live-test output.

A new technique needs decode, preflight, execute, and release callbacks under
`src/techniques/`. Register its selector, object name, options, and help in both
integrations, and declare only the common modules it uses in `Makefile`.

Put shared native API code under `src/common/`. Keep Windows API details out of
the CNA and Cobalt Strike parsing out of the BOF. Document requirements,
network ports, results, cleanup, and detection. Test new objects on a disposable
authorized target; a malformed BOF can terminate its implant process.

