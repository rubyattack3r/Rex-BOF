# Changelog

Notable changes to Rex-BOF are documented here. The project follows [Semantic Versioning](https://semver.org/); before 1.0, incompatible changes increment the minor version.

## [Unreleased]

## [0.2.0] - 2026-10-29

### Changed

- Replace the public Cobalt Strike and Sliver command names with lowercase
  kebab-case forms. Compatibility aliases are intentionally not retained;
  native selector and transcript tokens remain unchanged.

## [0.1.0] - 2026-10-29

### Added

- Standalone x86/x64 BOF objects for `WMIProvider`, `WMIService`, `WMIExec`, `WSManExec`, `TaskSchedExec`, `DCOMIShellDispatch2`, `SCMChangeService`, and `SCMCreateService`.
- The `rex` Cobalt Strike integration with technique-aware options, active-token identity, launcher contracts, Unicode arguments, and a read-only `preflight` action.
- Typed transcript results and explicit recovery context without automatic remote restoration or raw command retention.
- Repository-native technique, launcher, configuration, output, recovery, platform, and architecture documentation.
- Reproducible Nix development shell, verified release packaging, checksums,
  build provenance, offline operator documentation, and tag-driven GitHub
  releases.
- Sliver extension packaging with eight typed commands, x86/x64 object
  selection, built-in Reflektor execution, and a `coff-loader` fallback.

### Fixed

- Export the Sliver adapter entry points without x86 symbol decoration so
  Reflektor can resolve the exact manifest names on both architectures.

