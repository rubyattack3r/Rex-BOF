# Changelog

Notable changes to Rex-BOF are documented here. The project follows [Semantic Versioning](https://semver.org/); before 1.0, incompatible changes increment the minor version.

## [Unreleased]

### Added

- Standalone x86/x64 BOF objects for `WMIProvider`, `WMIService`, `WMIExec`, `WSManExec`, `TaskSchedExec`, `DCOMIShellDispatch2`, `SCMChangeService`, and `SCMCreateService`.
- The `rex` Cobalt Strike integration with technique-aware options, active-token identity, launcher contracts, Unicode arguments, and a read-only `preflight` action.
- Typed transcript results and explicit recovery context without automatic remote restoration or raw command retention.
- Repository-native technique, launcher, configuration, output, recovery, platform, and architecture documentation.

