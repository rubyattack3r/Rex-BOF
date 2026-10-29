# Contributing

By contributing, you agree to the [Code of Conduct](CODE_OF_CONDUCT.md) and [Acceptable Use Policy](ACCEPTABLE_USE_POLICY.md).

Create a focused branch, run `make verify`, and include tests and documentation with behavior changes. A new technique must provide separate decode, read-only preflight, execute, and release callbacks under `src/techniques/`; declare only its required common modules in `Makefile`; and register its stable selector, object basename, named options, and help in `integrations/cna/rex.cna`.

Document the technique's mechanism, privileges, network surface, success evidence, retained recovery state, and detection considerations. Build and inspect both architectures. Do not commit generated object files, operational payloads, target configuration, credentials, keys, or live-test output.

Shared native API lifecycle belongs under `src/common/`. Keep technique-specific native details out of the CNA and keep Cobalt Strike parsing or launcher composition out of the C BOF. A malformed BOF can terminate its containing process, so validate new objects against a disposable, explicitly authorized target.

