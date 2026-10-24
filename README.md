# BHunter Revived

A source restoration of BHunter, with the goal of faithfully playable gameplay on modern Windows.

**Early research stage: this is not a playable restored game yet.** The original executable is the behavioral reference. The project separates original game/engine recovery from verified runtime libraries and platform adapters. Functional fidelity is the default; complete instruction matching is supporting evidence rather than a release requirement.

## Progress

<!-- BHUNTER_STATUS_START -->

**Restored and linked source functions:** `[###---------------------]` **13.9% (101/727)**

| Mapped | Excluded runtime/helpers | Potential game/engine | Restored & linked | In progress | Not started |
|---:|---:|---:|---:|---:|---:|
| 887 | 160 | 727 | 101 | 8 | 618 |

**Remaining:** 626 potential functions. Current linked implementations: 101 functional / 0 byte-matched; 0 other adapted/reused addresses.

**Admitted into a playable game target:** `[------------------------]` **0.0% (0/727)**

The denominator uses the **full 887-function role catalog**, including unknown candidates, minus verified libraries, compiler helpers, imports and shared tails. It is provisional: further classification may exclude more functions. **Function count is not effort or playable-game completion.**

The ledger tracks 109 distinct eligible addresses across 128 implementation variants: 15 historical/superseded and 4 object-only byte-match variants are kept separate. Object-only matches do not replace linked functional evidence or add restored functions. 66 source support implementations are counted separately.

Sources: [role catalog](META/m1/role-catalog.json), [implementation ledger](META/function-status.json). Milestones: [progress and blockers](META/PROGRESS.md).

<!-- BHUNTER_STATUS_END -->

## Restoration approach

- Recovered code must compile, **link**, and execute relevant tests before admission.
- Classify game, engine, CRT/library, compiler helpers and shared tails before assigning recovery.
- Track byte matches, functional implementations, integration and validation separately.
- Build a faithful playable slice, extend feature coverage, then pass a source-only release audit.

The [restoration plan](RESTORATION_PLAN.md) defines milestones and exit gates. [Current progress and blockers](META/PROGRESS.md) distinguish experiments from completed integration. [Agent rules](AGENTS.md) preserve those constraints across work sessions.

M1's [initial frontier review](META/m1/REVIEW.md) now catalogs roles and dependency ownership, corrects runtime/data false targets, and defines the next archive/VFILE family. Unknown provenance and indirect dependencies remain explicit; this review adds no recovered gameplay implementations.

M2's [archive/VFILE family](META/m2/VFILE.md) now links nine functional variants with the math module. The real-data checkpoint compares all 1,229 archive payloads with independent raw reads, plus cache/line/lifecycle tests. The [math numerical contract](META/m2/MATH.md) now has explicit FP controls and expanded source-only tests. The [config/profile-container/path family](META/m2/CONFIG.md) now adds binary config persistence, explicit list ownership, device reconciliation and bounded startup paths. The [bounded scene-data readers](META/m2/SCENE.md) now decode every placement/link in TEST.LEV and every route cell in TEST.PTH through three source backends. The [BSP/block data family](META/m2/BLOCKS.md) now loads all 505 BSPs, preserves 202 unapplied scripts and resolves all level placements through a 454-model registry. The [actual entity pool/list family](META/m2/ENTITIES.md) now has seven linked functional variants. The [bounded WAV/PCM data reader](META/m2/WAVE.md) now compares every sample in all 60 WAVs through three backends. The [PCX/palette family](META/m2/PCX.md) now decodes all 459 images and preserves palette/index transformations. Full world, scripts, texture/audio objects and navigation behavior remain WIP; engine authorship and runtime transitions remain unresolved. The complete [M2 exit audit](META/m2/EXIT_AUDIT.md) passes: 376,611 real-data and 36,131 clean source-only checks, with 23 translation units and eleven missing-module controls. M3 is skipped; M4 decompilation and source restoration is active. No playable source game is claimed.

**M3 is skipped by user direction (2026-10-24), not passed. M4 decompilation and source restoration is active.** Existing [reference evidence](META/m3/REFERENCE.md) is retained with missing observations explicitly unvalidated. No further automated gameplay or pilot development; any game execution is limited to a bounded startup test. The next implementation deliverable is recovered source compiled in its real translation unit, linked with real definitions and exercised by relevant source-built checks. M4 source restoration is active; the latest source/build evidence is recorded in `META/PROGRESS.md`. Work remains CLI/programmatic without computer-use or visual tools.

## Current evidence

Four reconstructed math functions reproduce **335/335 original instruction bytes** in two clean combined builds with the pinned VC5 SP3 C++ setup. One function depends on the preserved translation-unit context. These are matching objects, not a playable game. See [matching findings](analysis/astra-review/FIXED_SETUP_FINDINGS.md).

Ten current C math variants link into the integration target under PC53/nearest/masked exceptions. Expanded oracle tests cover 566,784 function/alias comparisons across three precisions; finite bits match at PC53 and diagnostic PC64. The source-only numerical checkpoint passes 28,302 checks. PC24 inverse-point and NaN payload limits are explicit in the [current math evidence](META/m2/MATH.md); [historical results](analysis/math/RECOVERY.md) remain separate.

## Repository contents

| Directory | Purpose |
|---|---|
| `src/` | Reconstructed functional source and types |
| `META/` | Milestones, implementation-variant ledger and blockers |
| `analysis/` | Curated maps, comparison source and research evidence |
| `tools/` | Status generator and reviewed declaration-context scanner |

Original game files, disc images, SDK/compiler binaries, Ghidra projects and generated experiment trees remain local and are ignored by Git. [Build instructions](BUILDING.md) document the pinned local prerequisites. Run `build.cmd` for source-only math, archive/VFILE config/container/path, scene, BSP/block-data and entity-pool, WAV-reader and PCX/palette checks; add `--archive GAME_DATA/Bhunter.bin` for real-asset byte comparisons. All recovered data modules and the explicit float-store helper have missing-definition link controls. This is not a game build. [Current clean checkout evidence](META/m2/pcx-clean/clean-checkout.json) records a successful build without original game files.

## Update status

With Python 3.10 or newer installed:

```cmd
mkstatus.cmd
```

The command works from any working directory. It reads `META/m1/role-catalog.json` and `META/function-status.json`, counts the full mapped function pool rather than only tracked implementations, and updates only the marked section above. Run it after recording new evidence; it does not infer correctness from source files or regenerate the milestone report.
