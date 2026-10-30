# Changelog

Notable changes to Rex-BOF are documented here. The project follows
[Semantic Versioning](https://semver.org/); before 1.0, incompatible changes
increment the minor version.

## [Unreleased]

### Added

- Add `wmi-instance-provider` and `wmi-method-provider` as explicit x86 and
  x64 Cobalt Strike and Sliver techniques with typed role registration and
  recovery context.
- Add optional command-scoped make-token credentials to every Cobalt Strike and
  Sliver technique. Down-level and UPN principals are supported without
  changing the implant task thread's token.
- Add x86 and x64 standalone Windows executables built from the shared
  technique registry and lifecycle, with inline passwords or an interactive
  no-echo password prompt.

### Changed

- Document the provider DLL, service executable, and WMI event-consumer command
  contracts used by techniques that do not resolve launchers.
- Share provider registration and lifecycle code across the initialization,
  instance, and method provider techniques.
- Release a verified standalone ZIP alongside the Cobalt Strike and Sliver
  bundles.

## [0.4.2] - 2026-10-29

### Fixed

- Initialize COM security with dynamic cloaking before WMI connections and
  DCOM activation so remote calls honor the active token context.

## [0.4.1] - 2026-10-29

### Fixed

- Return scalar booleans from the Cobalt Strike CNA predicate helpers so
  canonical techniques and their named options are accepted by Sleep.

## [0.4.0] - 2026-10-29

### Added

- Added `wmi-event-subscription` and `scm-command` for x86 and x64 Cobalt
  Strike and Sliver consumers.
- Added a platform-validation column to the technique reference and a shared
  environment registry for techniques and launchers.
- Added a repository-wide `clang-format` policy and enforced it in
  `make verify`.

### Changed

- Make the shared decode, preflight, execute, and release lifecycle explicit
  and record the Server 2025 remote-activation boundary for
  `dcom-shell-browser-window`.
- Rewrite operator and developer documentation for clearer installation,
  requirements, results, and cleanup guidance. Launcher entries now link
  directly to their LOLBAS references. Every documentation directory now uses
  `README.md` as its automatic GitHub landing page.
- Use Zig for reproducible x86 and x64 Windows BOF builds, with the same target
  and BOF-oriented compiler conventions as Zip-BOF. Each technique now builds
  as one translation unit and is stripped and inspected with LLVM tools.
- Withhold `dcom-mmc20` and `dcom-shell-browser-window` from release bundles.
  Server 2025 validation found an in-process MMC automation hang and a
  platform-level `ShellBrowserWindow` activation failure.
- Remove the unvalidated Sliver `coff-loader` fallback. Sliver now requires
  built-in Reflektor support and fails closed on older clients.

### Fixed

- Allow canonical kebab-case selectors through the shared native input
  validator and guard the contract with a verification check.
- Construct technique descriptors in caller-owned storage and disable pointer
  jump tables so x64 BOFs contain no Cobalt Strike-incompatible
  `IMAGE_REL_AMD64_ADDR64` or `.refptr` relocations. Object verification now
  rejects either form.

## [0.3.0] - 2026-10-29

### Changed

- Use canonical lowercase kebab-case selectors throughout the native request,
  both integrations, and the transcript `technique` field.

## [0.2.0] - 2026-10-29

### Changed

- Replace the public Cobalt Strike and Sliver command names with lowercase
  kebab-case forms. Compatibility aliases are intentionally not retained;
  native selector and transcript tokens remain unchanged.

## [0.1.0] - 2026-10-29

### Added

- Standalone x86/x64 BOF objects for `WMIProvider`, `WMIService`, `WMIExec`,
  `WSManExec`, `TaskSchedExec`, `DCOMIShellDispatch2`, `SCMChangeService`, and
  `SCMCreateService`.
- The `rex` Cobalt Strike integration with technique-aware options,
  active-token identity, launcher contracts, Unicode arguments, and a
  read-only `preflight` action.
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

