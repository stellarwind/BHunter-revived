# BHunter restoration progress

Updated 2026-10-23. Plan: `../RESTORATION_PLAN.md`.

## Current checkpoint

**Reconnaissance and math experiments; no playable source-built game yet.** M0 is the next work block. This plan has not created a build system or completed the integration gates.

| Milestone | State | Evidence / next gate |
|---|---|---|
| M0 Reference/build baseline | Open; repository baseline established | Git repository created with source/research and status generation; named clean integration build and dependency manifest still needed |
| M1 Provenance/source map | Preliminary | 887 observed records, 147 verified CRT functions, 243 candidate callback boundaries; first-slice roles/dependency closure needs review |
| M2 Foundations integrated | Partial experimental harness only | Ten math routines link into differential harness; four matching implementations are a different, object-only variant |
| M3 Runnable original reference/scenarios | Open | SourceDir/copyright check diagnosed; full playable reference and media/feature inventory not established |
| M4 First source-driven playable slice | Not started | No source game target |
| M5 Complete single-player | Not started | Requires slice and actual original feature inventory |
| M6 Modern platform support | Not started | Backend choice follows faithful slice; numerical contracts remain open |
| M7 Verified networking | Not started | Active DirectPlay3A references; modes/protocol still need tracing |
| M8 Source-only audit | Not started | Not measurable from current instruction coverage |
| M9 Release/preservation | Not started | No release package |

## Working evidence

- Original English EXE SHA256: `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
- Four matching functions: `analysis/astra-review/matched_math.cpp`; two clean combined builds reproduce 335/335 raw bytes. Direction's isolated form differs; preserve the combined context.
- Positive-control command: `python analysis/astra-review/verify_fixed_math.py`.
- Functional C variant: `src/bh_math.c`; native differential command `powershell -File analysis/math/build_and_test.ps1`. 60,000 finite fixtures, no tolerance failures; 24-bit bit differences on five routines remain unresolved.
- Padscan adaptation reviewed against real VC5 output; baseline 4/4 raw matches, source unchanged; declaration changes affect emitted code. Initial evidence: `analysis/padscan/run-b6420c063204/results.json` and `run-2ba29f93c1fe/results.json`.
- Explicit positive-control gate also passed: `analysis/padscan/run-3dc5596e4e33/results.json`. Translation remains exact across all five tested declaration counts.
- Library survey has not identified QvLib or another reusable open-source engine library in BHunter. Required unknown engine behavior must still be covered.

## Open blockers / admission conditions

1. No named continuously linked integration target yet. Version control now preserves the source/research baseline; it does not complete M0.
2. SP3 linker loader failure; harness currently uses stock VC5 LINK. Record and reproduce the split before claiming build baseline complete.
3. Existing matching objects have not been integrated into the functional C harness or a game target. They remain separate implementation variants.
4. Runtime x87 precision transitions and 24-bit reconstruction discrepancies are unverified.
5. Candidate callbacks, shared tails and compiler/unwind helper roles require review on each assigned frontier.
6. Original reference launch, actual media tracks, full feature inventory and multiplayer behavior are not yet established.

No new byte-match sweep is required before starting M0. No milestone is complete merely because the plan or source file exists.

## Repository and status tooling

The repository is named `BHunter-revived`. Original game/media, local toolchains, Ghidra projects and large generated runs are excluded. Small curated evidence snapshots are retained under `analysis/evidence/`.

Run `mkstatus.cmd` to update the README from the implementation ledger. It deduplicates original addresses across variants, distinguishes `bm` and `func`, excludes library/compiler roles and displays game admission separately. Current tracked sample: 10 functions, four `bm`, three `func`, three `wip`; zero admitted into a game target. This is not a whole-game completion denominator.
