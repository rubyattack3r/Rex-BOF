# Repository guidance

## Session wiki

`.wiki/` is gitignored cross-session memory. It is never shipped or committed,
and shipped documentation must not link to it. Read relevant pages before work
when they exist. Before handing off, update the owning page with durable current
truth and source-tree evidence rather than logs, prompts, transcripts, or issue
discussion.

- `index.md`: one link and one-line summary per page.
- `log.md`: distilled dated changes and rationale; keep about 20 entries.
- `environment.md`: test hosts and access patterns, never credentials or keys.
- `architecture.md`: package boundaries and runtime contracts.
- `requirements.md`: approved scope and milestone decisions.
- `findings.md`: problem → cause → fix; remove superseded findings.
- `next-steps.md`: actionable follow-up, blockers, and pending verification.

## Scratch workspace

`.scratch/` is gitignored disposable space for code spikes, downloaded tools,
generated binaries, rendered output, and test logs. It may be deleted at any
time and is never a source of truth or a secret store. Promote durable code into
the tracked tree and durable context into `.wiki/`.

## Project layout

- `include/` owns shared public C contracts and Beacon declarations.
- `src/common/` owns the entrypoint, typed results, transcript encoding, and
  reusable WMI and service capabilities.
- `src/techniques/` owns one implementation and descriptor per stable selector.
- `integrations/cna/rex.cna` owns the Cobalt Strike command surface, launcher
  resolution, architecture selection, and request packing.
- `docs/` owns repository-native operator and developer documentation.
- `tests/` contains deterministic native unit tests.

Keep the CNA independent of native API details. Preflight must be read-only and
complete before mutation. Process-local resources are always released; retained
remote state must be represented in typed recovery context without commands,
arguments, credentials, or payload contents.

## Validation

Run `make verify` for clean x86/x64 builds, the undefined-import allowlist, and
documentation checks. CI runs the transcript encoder test with MSVC on Windows.
When available, run `make verify-bof BOFLINT=/path/to/boflint.py` against every
generated object. Update code, CNA help, operator documentation, recovery
guidance, and detection considerations together when a public contract changes.

