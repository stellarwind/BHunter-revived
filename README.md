# BHunter Revived

A source restoration of BHunter, with the goal of faithfully playable gameplay on modern Windows.

**Early research stage: this is not a playable restored game yet.** The original executable is the behavioral reference. The project separates original game/engine recovery from verified runtime libraries and platform adapters. Functional fidelity is the default; complete instruction matching is supporting evidence rather than a release requirement.

## Progress

<!-- BHUNTER_STATUS_START -->

**Tracked recovery evidence — catalogued functions only:** `[################--------]` **70% (7/10)**

| Byte-matched | Functional | In progress | Not started | Adapters / reused |
|---:|---:|---:|---:|---:|
| 4 | 3 | 3 | 0 | 0 / 0 |

**Admitted into the game target:** `[------------------------]` **0% (0/10)**

This bar covers **10 catalogued game/engine function addresses**, deduplicated from 14 implementation records. It is **not overall game completion**. A byte-matched object can remain unintegrated. Verified CRT/library/compiler-support records are excluded; adapters/reused dependencies are shown separately from `bm`/`func`.

Source: [implementation ledger](META/function-status.json). Milestones: [progress and blockers](META/PROGRESS.md).

<!-- BHUNTER_STATUS_END -->

## Restoration approach

- Recovered code must compile, **link**, and execute relevant tests before admission.
- Classify game, engine, CRT/library, compiler helpers and shared tails before assigning recovery.
- Track byte matches, functional implementations, integration and validation separately.
- Build a faithful playable slice, extend feature coverage, then pass a source-only release audit.

The [restoration plan](RESTORATION_PLAN.md) defines milestones and exit gates. [Current progress and blockers](META/PROGRESS.md) distinguish experiments from completed integration. [Agent rules](AGENTS.md) preserve those constraints across work sessions.

## Current evidence

Four reconstructed math functions reproduce **335/335 original instruction bytes** in two clean combined builds with the pinned VC5 SP3 C++ setup. One function depends on the preserved translation-unit context. These are matching objects, not a playable game. See [matching findings](analysis/astra-review/FIXED_SETUP_FINDINGS.md).

Ten functional math reconstructions have been linked into a differential harness. The existing finite fixtures match at 53/64-bit x87 precision; some 24-bit rounding differences remain open. See [recovery limits and results](analysis/math/RECOVERY.md).

## Repository contents

| Directory | Purpose |
|---|---|
| `src/` | Reconstructed functional source and types |
| `META/` | Milestones, implementation-variant ledger and blockers |
| `analysis/` | Curated maps, comparison source and research evidence |
| `tools/` | Status generator and reviewed declaration-context scanner |

Original game files, disc images, SDK/compiler binaries, Ghidra projects and generated experiment trees remain local and are ignored by Git. [Analysis setup notes](analysis/README.md) describe the current local prerequisites; there is no turnkey game build yet.

## Update status

With Python 3.10 or newer installed:

```cmd
mkstatus.cmd
```

The command works from any working directory. It reads `META/function-status.json`, counts original function addresses rather than source variants, and updates only the marked section above. Run it after recording new evidence; it does not infer correctness from source files or regenerate the milestone report.
