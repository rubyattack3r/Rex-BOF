# Rex BOF project context

## Overview

This repository builds eight Windows remote-execution techniques as native
Cobalt Strike BOFs. Every technique supports x86 and x64 Beacons. Keep the
typed runtime and reusable WMI and service code shared, with one selector,
implementation, descriptor, and object per technique. Cobalt Strike and Sliver
integrations live separately under `integrations/`.

## Architecture

- `include/rex.h` defines the shared request, result, observation, recovery,
  state, and technique-descriptor contracts. `include/beacon.h` declares the
  Beacon APIs available to the objects.
- `src/common/rex.c` is the common BOF entry point. `src/common/result.c`,
  `runtime.c`, and `text.c` own typed output, runtime helpers, and UTF-8
  transcript encoding. `wmi.c` and `service.c` provide reusable capabilities
  linked only into techniques that need them.
- `src/techniques/` contains one implementation and one `rex_technique`
  descriptor for each canonical stable selector: `wmi-provider`, `wmi-service`,
  `wmi-exec`, `wsman-exec`, `task-sched-exec`, `dcom-ishell-dispatch2`,
  `scm-change-service`, and `scm-create-service`.
- Public Cobalt Strike commands use `wmi-provider`, `wmi-service`, `wmi-exec`,
  `wsman-exec`, `task-sched-exec`, `dcom-ishell-dispatch2`,
  `scm-change-service`, and `scm-create-service`. Sliver prefixes the same
  names with `rex-`. Do not add compatibility aliases. Native requests and
  transcripts use the same canonical kebab-case selectors.
- Each BOF is linked from the common modules and exactly one technique module.
  The common entry point rejects a packed selector that does not match the
  linked descriptor.
- `integrations/cna/rex.cna` owns the operator command surface, named-option
  parsing, launcher resolution, request packing, and x86/x64 selection. Keep
  native API details out of the CNA. The generated objects remain adjacent to
  the CNA so `script_resource` can resolve them.
- `integrations/sliver/extension.json` owns the Sliver command surface, typed
  argument declarations, BOF executor selection, and `amd64`/`386` selection.
  It exposes primitive fields directly and does not reproduce CNA launcher
  resolution.
- `docs/integrations/` contains the Cobalt Strike and Sliver operator guides;
  the rest of `docs/` owns shared operator and developer documentation.
  `tests/` contains deterministic native tests. `scripts/` contains
  documentation and release checks. `.github/` contains CI, CodeQL, release
  workflows, Dependabot, and community health files.
- The Cobalt Strike and Sliver release bundles each contain their integration
  file, all 16 objects, the offline `docs/` tree, project metadata, the MIT
  license, and acceptable-use guidance. Each bundle keeps its integration file
  and objects together at its root.

## Behavior and Safety Invariants

- Preserve the eight public kebab-case command names, internal stable
  selectors, and documented named options unless a user-facing change is
  intended. Keep one target per invocation and one object per selector and
  architecture.
- Each integration and its objects form one versioned request contract. Do not
  mix builds. Both integrations use the active implant token and must not
  accept credentials or change token state.
- Preflight must be read-only and finish before any mutation. It validates the
  exact request that execution would consume. Rex does not stage, inspect, or
  remove referenced payload files.
- Every technique implements decode, preflight, execute, and release callbacks.
  Release must tolerate partial initialization and run after preflight-only,
  success, and failure paths. Always release process-local handles, interfaces,
  sessions, and allocations.
- Record retained remote state in typed recovery context as soon as it exists.
  Do not retain commands, arguments, credentials, or payload contents. Keep
  recovery instructions explicit and technique-specific.
- Preserve result semantics: `validated` proves preflight only, `dispatched`
  proves that a remote API accepted work, and `completed` represents a
  synchronous successful outcome. Never equate dispatch with payload success.
- Declare only Beacon APIs and dynamic `MODULE$function` imports. Do not add a
  C runtime dependency or direct Windows import. The undefined-import check
  rejects symbols outside the allowlist.
- Keep operator text and transcript fields UTF-8 and Windows native arguments
  UTF-16. Preserve percent-encoding for ambiguous transcript values and the
  documented target, command, and field limits. Keep transcript `technique`
  values aligned with the canonical public kebab-case names.
- Changes to request packing, decoding, preflight, recovery, or output require
  matching CNA help, Sliver manifest arguments, operator documentation,
  detection considerations, and focused x86/x64 validation.

## Build and verification

Use the `flake.nix` development shell; MinGW-w64 compiles and links both
architectures. Run these checks after relevant changes:

```sh
nix develop -c make verify
nix develop -c make release VERSION=v0.0.0-ci
```

`make verify` runs cppcheck, shellcheck, and actionlint, performs clean x86/x64
builds, rejects unexpected undefined imports, and checks documentation links
and required technique diagrams. It also verifies that each Sliver object
defines the manifest entry point with the exact undecorated name on both
architectures. The release target creates deterministic Cobalt Strike and
Sliver ZIPs, verifies their exact contents, extracts them and checks their
offline documentation, validates the Sliver manifest and entry points, and
writes SHA-256 checksums. CI also compiles and runs the transcript encoder test
with MSVC on Windows.

When `boflint.py` is available, run this against every generated object:

```sh
make verify-bof BOFLINT=/path/to/boflint.py
```

For behavior native tests cannot cover, validate both architectures in a
disposable authorized Windows environment and record the Windows, Cobalt
Strike, Sliver, and implant versions used.

## Documentation and changelog

- Keep `README.md`, CNA help, the Sliver manifest, `docs/`, tested-platform
  evidence, and release contents consistent with behavior. A release consumer
  must be able to use every relative documentation link offline.
- Follow Keep a Changelog. Put user-facing changes under `Unreleased`, grouped
  by applicable types such as Added, Changed, Fixed, and Security. Move them to
  a dated version section only when releasing. Do not use `CHANGELOG.md` as a
  work log.
- Keep platform and loader claims tied to recorded tests. Do not infer a
  minimum Cobalt Strike or Sliver version without evidence.

## Releasing

1. Move the `Unreleased` entries in `CHANGELOG.md` to a new
   `## [X.Y.Z] - YYYY-MM-DD` section.
2. Run `nix develop -c make verify` and
   `nix develop -c make release VERSION=vX.Y.Z`.
3. Commit, then tag and push: `git tag vX.Y.Z && git push origin vX.Y.Z`.
4. The release workflow rebuilds both bundles, publishes the ZIPs and checksums
   using that changelog section as release notes, and attests build provenance.
   It fails when the changelog has no section matching the tag.

Pin every GitHub Action to a commit SHA with a `# ratchet:` comment.

